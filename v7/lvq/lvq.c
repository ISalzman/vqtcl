/*  Lua extension binding.
    $Id$
    This file is part of Vlerq, see lvq/vlerq.h for full copyright notice. */

#include <lauxlib.h>

#include "vlerq.h"
#include "defs.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* forward */
static vqView checkview (lua_State *L, int t);
static vqView table2view (lua_State *L, int t);
    
static void *alloc_vec (const vqDispatch *vtab, int bytes) {
    char *result = (char*) calloc(1, bytes + vtab->prefix) + vtab->prefix;
    vDisp(result) = vtab;
    return result;
}
void *vq_incref (void *v) {
    if (v != 0)
        ++vRefs(v); /* TODO: check for overflow */
    return v;
}
void vq_decref (void *v) {
    if (v != 0 && --vRefs(v) <= 0) {
        /*assert(vRefs(v) == 0);*/
        if (vDisp(v)->cleaner != 0)
            vDisp(v)->cleaner(v);
        free((char*) v - vDisp(v)->prefix);
    }
}

static void *tagged_udata (lua_State *L, size_t bytes, const char *tag) {
    void *p = lua_newuserdata(L, bytes);
    luaL_getmetatable(L, tag);
    lua_setmetatable(L, -2);
    return p;
}

static vqView empty_meta (lua_State *L) {
    vqView v;
    lua_getfield(L, LUA_REGISTRYINDEX, "lvq.emv"); /* t */
    v = checkview(L, -1);
    lua_pop(L, 1);
    return v;
}

static int char2type (char c) {
    const char *p = strchr(VQ_TYPES, c & ~0x20);
    int type = p != 0 ? p - VQ_TYPES : VQ_nil;
    if (c & 0x20)
        type |= VQ_NULLABLE;
    return type;
}
/*
static int string2type (const char *str) {
    int type = char2type(*str);
    while (*++str != 0)
        if ('a' <= *str && *str <= 'z')
        type |= 1 << (*str - 'a' + 5);
    return type;
}
*/
static const char* type2string (int type, char *buf) {
    char c, *p = buf; /* buffer should have room for at least 28 bytes */
    *p++ = VQ_TYPES[type&VQ_TYPEMASK];
    if (type & VQ_NULLABLE)
        p[-1] |= 0x20;
    for (c = 'a'; c <= 'z'; ++c)
        if (type & (1 << (c - 'a' + 5)))
            *p++ = c;
    *p = 0;
    return buf;
}

static int push_view (vqView v) {
    lua_State *L = vwState(v);
    lua_getfield(L, LUA_REGISTRYINDEX, "lvq.pool");
    lua_pushlightuserdata(L, v);
    lua_rawget(L, -2);
    /* create and store a new lvq.view object if there wasn't one */
    if (lua_isnil(L, -1)) {
        vqView *ud = tagged_udata(L, sizeof *ud, "lvq.view");
        *ud = vq_incref(v);
        lua_pushlightuserdata(L, v);
        lua_pushvalue(L, -2);
        lua_rawset(L, -5);
        lua_remove(L, -2);
    }
    lua_remove(L, -2);
    return 1;
}

static vqView desc_parser (vqView emv, char **desc, const char **nbuf, int *tbuf, vqView *sbuf) {
    char sep, *ptr = *desc;
    const char  **names = nbuf;
    int c, nc = 0, *types = tbuf;
    vqView result, *subvs = sbuf;
    
    for (;;) {
        const char* s = ptr;
        sbuf[nc] = emv;
        tbuf[nc] = VQ_string;
        
        while (strchr(":,[]", *ptr) == 0)
            ++ptr;
        sep = *ptr;
        *ptr = 0;

        if (sep == '[') {
            ++ptr;
            sbuf[nc] = desc_parser(emv, &ptr, nbuf+nc, tbuf+nc, sbuf+nc);
            tbuf[nc] = VQ_view;
            sep = *++ptr;
        } else if (sep == ':') {
            tbuf[nc] = char2type(*++ptr);
            sep = *++ptr;
        }
        
        nbuf[nc++] = s;
        if (sep != ',')
            break;
            
        ++ptr;
    }
    
    *desc = ptr;

    result = vq_new(vwMeta(emv), nc);

    for (c = 0; c < nc; ++c)
        vq_setMetaRow(result, c, names[c], types[c], subvs[c]);
    
    return result;
}
static vqView desc2meta (lua_State *L, const char *desc, int length) {
    int i, bytes, limit = 1;
    void *buffer;
    const char **nbuf;
    int *tbuf;
    vqView *sbuf, meta;
    char *dbuf;
    vqView emv = empty_meta(L);
    
    if (length == 0)
        return emv;

    if (length < 0)
        length = strlen(desc);
    
    /* find a hard upper bound for the buffer requirements */
    for (i = 0; i < length; ++i)
        if (desc[i] == ',' || desc[i] == '[')
            ++limit;
    
    bytes = limit * (2 * sizeof(void*) + sizeof(int)) + length + 1;
    buffer = malloc(bytes);
    nbuf = buffer;
    tbuf = (void*) (nbuf + limit);
    sbuf = (void*) (tbuf + limit);
    dbuf = memcpy(sbuf + limit, desc, length);
    dbuf[length] = ']';
    
    meta = desc_parser(emv, &dbuf, nbuf, tbuf, sbuf);
    
    free(buffer);
    return meta;
}

static vqType NilVecGetter (int row, vqCell *cp) {
    const char *p = cp->p;
    cp->i = (p[row/8] >> (row&7)) & 1;
    return cp->i ? VQ_int : VQ_nil;
}
static vqType IntVecGetter (int row, vqCell *cp) {
    const int *p = cp->p;
    cp->i = p[row];
    return VQ_int;
}
static vqType LongVecGetter (int row, vqCell *cp) {
    const int64_t *p = cp->p;
    cp->l = p[row];
    return VQ_long;
}
static vqType FloatVecGetter (int row, vqCell *cp) {
    const float *p = cp->p;
    cp->f = p[row];
    return VQ_float;
}
static vqType DoubleVecGetter (int row, vqCell *cp) {
    const double *p = cp->p;
    cp->d = p[row];
    return VQ_double;
}
static vqType StringVecGetter (int row, vqCell *cp) {
    const char **p = cp->p;
    cp->s = p[row] != 0 ? p[row] : "";
    return VQ_string;
}
static vqType BytesVecGetter (int row, vqCell *cp) {
    vqCell *p = cp->p;
    *cp = p[row];
    return VQ_bytes;
}
static vqType ViewVecGetter (int row, vqCell *cp) {
    vqView *p = cp->p;
    cp->v = p[row];
    return VQ_view;
}

static void NilVecSetter (void *q, int row, int col, const vqCell *cp) {
    char *p = q;
    int bit = 1 << (row&7);
    VQ_UNUSED(col);
    if (cp != 0)
        p[row/8] |= bit;
    else
        p[row/8] &= ~bit;
}
static void IntVecSetter (void *q, int row, int col, const vqCell *cp) {
    int *p = q;
    VQ_UNUSED(col);
    p[row] = cp != 0 ? cp->i : 0;
}
static void LongVecSetter (void *q, int row, int col, const vqCell *cp) {
    int64_t *p = q;
    VQ_UNUSED(col);
    p[row] = cp != 0 ? cp->l : 0;
}
static void FloatVecSetter (void *q, int row, int col, const vqCell *cp) {
    float *p = q;
    VQ_UNUSED(col);
    p[row] = cp != 0 ? cp->f : 0;
}
static void DoubleVecSetter (void *q, int row, int col, const vqCell *cp) {
    double *p = q;
    VQ_UNUSED(col);
    p[row] = cp != 0 ? cp->d : 0;
}
static void StringVecSetter (void *q, int row, int col, const vqCell *cp) {
    const char **p = q;
    VQ_UNUSED(col);
    free((void*) p[row]);
    p[row] = cp != 0 ? strcpy(malloc(strlen(cp->s)+1), cp->s) : 0;
}
static void BytesVecSetter (void *q, int row, int col, const vqCell *cp) {
    vqCell *p = q;
    VQ_UNUSED(col);
    free(p[row].p);
    if (cp != 0) {
        p[row].p = memcpy(malloc(cp->x.y.i), cp->p, cp->x.y.i);
        p[row].x.y.i = cp->x.y.i;
    } else {
        p[row].p = 0;
        p[row].x.y.i = 0;
    }
}
static void ViewVecSetter (void *q, int row, int col, const vqCell *cp) {
    vqView *p = q;
    VQ_UNUSED(col);
    vq_incref(cp->v);
    vq_decref(p[row]);
    p[row] = cp->v;
}

static void StringVecCleaner (void *q) {
    int i;
    const char **p = q;
    for (i = 0; i < vSize(q); ++i)
        free((void*) p[i]);
}
static void BytesVecCleaner (void *q) {
    int i;
    vqCell *p = q;
    for (i = 0; i < vSize(q); ++i)
        free(p[i].p);
}
static void ViewVecCleaner (void *q) {
    int i;
    vqView *p = q;
    for (i = 0; i < vSize(q); ++i)
        vq_decref(p[i]);
}

static vqDispatch nvtab = {
    "nilvec", sizeof(vqInfo), -1, 0, NilVecGetter, NilVecSetter
};
static vqDispatch ivtab = {
    "intvec", sizeof(vqInfo), 4, 0, IntVecGetter, IntVecSetter
};
static vqDispatch lvtab = {
    "longvec", sizeof(vqInfo), 8, 0, LongVecGetter, LongVecSetter 
};
static vqDispatch fvtab = {
    "floatvec", sizeof(vqInfo), 4, 0, FloatVecGetter, FloatVecSetter 
};
static vqDispatch dvtab = {
    "doublevec", sizeof(vqInfo), 8, 0, DoubleVecGetter, DoubleVecSetter 
};
static vqDispatch svtab = {
    "stringvec", sizeof(vqInfo), sizeof(void*), 
    StringVecCleaner, StringVecGetter, StringVecSetter
};
static vqDispatch bvtab = {
    "bytesvec", sizeof(vqInfo), sizeof(vqCell), 
    BytesVecCleaner, BytesVecGetter, BytesVecSetter
};
static vqDispatch tvtab = {
    "viewvec", sizeof(vqInfo), sizeof(void*), 
    ViewVecCleaner, ViewVecGetter, ViewVecSetter
};

static void *new_datavec (vqType type, int rows) {
    int bytes;
    vqDispatch *vtab;
    vqVec v;
    
    switch (type) {
        case VQ_nil:
            vtab = &nvtab; bytes = ((rows+31)/32) * sizeof(int); break;
        case VQ_int:
            vtab = &ivtab; bytes = rows * sizeof(int); break;
        case VQ_long:
            vtab = &lvtab; bytes = rows * sizeof(int64_t); break;
        case VQ_float:
            vtab = &fvtab; bytes = rows * sizeof(float); break;
        case VQ_double:
            vtab = &dvtab; bytes = rows * sizeof(double); break;
        case VQ_string:
            vtab = &svtab; bytes = rows * sizeof(const char*); break;
        case VQ_bytes:
            vtab = &bvtab; bytes = rows * sizeof(vqCell); break;
        case VQ_view:
            vtab = &tvtab; bytes = rows * sizeof(vqView*); break;
        default: assert(0); return 0;
    }
    
    v = alloc_vec(vtab, bytes);
    vSize(v) = vExtra(v) = rows;
    return vq_incref(v);
}

static void ViewCleaner (void *p) {
    vqView v = p;
    int c, nc = vwCols(v);
    for (c = 0; c < nc; ++c)
        vq_decref(vwCol(v,c).v);
    vq_decref(vwMeta(v));
}
static vqDispatch vtab = { "view", sizeof(struct vqView_s), 0, ViewCleaner };
static vqView alloc_view (const vqDispatch *vtabp, vqView meta, int rows, int extra) {
    vqView v = alloc_vec(vtabp, vwRows(meta) * sizeof(vqCell) + extra);
    assert(vtabp->prefix >= 3);
    vwState(v) = vwState(meta);
    vwAuxP(v) = (vqCell*) v + vwRows(meta);
    vwRows(v) = rows;
    vwMeta(v) = vq_incref(meta);
    return v;
}
vqView vq_new (vqView meta, int rows) {
    vqView v = alloc_view(&vtab, meta, rows, 0);
    if (rows > 0) {
        int c, nc = vwRows(meta);
        for (c = 0; c < nc; ++c) {
            vqType type = vq_getInt(meta, c, 1, VQ_nil) & VQ_TYPEMASK;
            vwCol(v,c).v = new_datavec(type, rows);
        }
    }
    
    return v;
}

int vq_getInt (vqView t, int row, int col, int def) {
    vqCell cell;
    cell.i = def;
    cell = vq_get(t, row, col, VQ_int, cell);
    return cell.i;
}
const char *vq_getString (vqView t, int row, int col, const char *def) {
    vqCell cell;
    cell.s = def;
    cell = vq_get(t, row, col, VQ_string, cell);
    return cell.s;
}
vqView vq_getView (vqView t, int row, int col, vqView def) {
    vqCell cell;
    cell.v = def;
    cell = vq_get(t, row, col, VQ_view, cell);
    return cell.v;
}

void vq_setEmpty (vqView t, int row, int col) {
    vqCell cell;
    vq_set(t, row, col, VQ_nil, cell);
}
void vq_setInt (vqView t, int row, int col, int val) {
    vqCell cell;
    cell.i = val;
    vq_set(t, row, col, VQ_int, cell);
}
void vq_setString (vqView t, int row, int col, const char *val) {
    vqCell cell;
    cell.s = val;
    vq_set(t, row, col, val != 0 ? VQ_string : VQ_nil, cell);
}
void vq_setView (vqView t, int row, int col, vqView val) {
    vqCell cell;
    cell.v = val;
    vq_set(t, row, col, val != 0 ? VQ_view : VQ_nil, cell);
}
void vq_setMetaRow (vqView m, int row, const char *nam, int typ, vqView sub) {
    vq_setString(m, row, 0, nam);
    vq_setInt(m, row, 1, typ);
    vq_setView(m, row, 2, sub != 0 ? sub : empty_meta(vwState(m)));
}

static void init_empty (lua_State *L) {
    vqView meta, mm;
    mm = alloc_vec(&vtab, 3 * sizeof(vqCell));
    vwState(mm) = L;
    vwRows(mm) = 3;
    vwMeta(mm) = mm;
    vwCol(mm,0).v = new_datavec(VQ_string, 3);
    vwCol(mm,1).v = new_datavec(VQ_int, 3);
    vwCol(mm,2).v = new_datavec(VQ_view, 3);
    
    meta = vq_new(mm, 0);
    push_view(meta);
    lua_setfield(L, LUA_REGISTRYINDEX, "lvq.emv");

    vq_setMetaRow(mm, 0, "name", VQ_string, meta);
    vq_setMetaRow(mm, 1, "type", VQ_int, meta);
    vq_setMetaRow(mm, 2, "subv", VQ_view, meta);
}

static vqType getcell (int row, vqCell *cp) {
    vqVec v = cp->p;
    if (v == 0)
        return VQ_nil;
    assert(vDisp(v)->getter != 0);
    return vDisp(v)->getter(row, cp);
}

int vq_isnil (vqView v, int row, int col) {
    vqCell cell;
    if (col < 0 || col >= vwCols(v))
        return 1;
    cell = vwCol(v, col);
    return getcell(row, &cell) == VQ_nil;
}
vqCell vq_get (vqView v, int row, int col, vqType type, vqCell def) {
    vqCell cell;
    VQ_UNUSED(type); /* TODO: check type arg */
    if (row < 0 || row >= vwRows(v) || col < 0 || col >= vwCols(v))
        return def;
    cell = vwCol(v, col);
    return getcell(row, &cell) == VQ_nil ? def : cell;
}
vqView vq_set (vqView v, int row, int col, vqType type, vqCell val) {
    if (vDisp(v)->setter == 0)
        v = vwCol(v,col).v;
    assert(vDisp(v)->setter != 0);
    vDisp(v)->setter(v, row, col, type != VQ_nil ? &val : 0);
    return v;
}
vqView vq_replace (vqView v, int start, int count, vqView data) {
    assert(vDisp(v)->replacer != 0);
    vDisp(v)->replacer(v, start, count, data);
    return v;
}

static vqCell *checkrow (lua_State *L, int t) {
    return luaL_checkudata(L, t, "lvq.row");
}

static vqView checkview (lua_State *L, int t) {
    switch (lua_type(L, t)) {
        case LUA_TNIL:      return 0;
        case LUA_TBOOLEAN:  return vq_new(empty_meta(L), lua_toboolean(L, t));
        case LUA_TNUMBER:   return vq_new(empty_meta(L), lua_tointeger(L, t));
        case LUA_TSTRING:   return desc2meta(L, lua_tostring(L, t), -1);
        case LUA_TTABLE:    return table2view(L, t);
    }
    return *(vqView*) luaL_checkudata(L, t, "lvq.view");
}

static int pushcell (lua_State *L, char c, vqCell *cp) {
    if (cp == 0 || c == 'N')
        return 0;
        
    switch (c) {
        case 'I':   lua_pushinteger(L, cp->i); break;
        case 'L':   lua_pushnumber(L, cp->l); break;
        case 'F':   lua_pushnumber(L, cp->f); break;
        case 'D':   lua_pushnumber(L, cp->d); break;
        case 'S':   lua_pushstring(L, cp->s); break;
        case 'B':   lua_pushlstring(L, cp->p, cp->x.y.i); break;
        case 'V':   push_view(cp->v); break;
    /* pseudo-types */
        case 'T':   lua_pushboolean(L, cp->i); break;
        default:    assert(0);
    }

    return 1;
}

static vqType check_cell (lua_State *L, int t, char c, vqCell *cp) {
    if ('a' <= c && c <= 'z') {
        if (lua_isnoneornil(L, t)) {
            cp->p = cp->x.y.p = 0;
            return VQ_nil;
        }
        c += 'A'-'a';
    }
    switch (c) {
        case 'N':   break;
        case 'I':   cp->i = luaL_checkinteger(L, t); break;
        case 'L':   cp->l = (int64_t) luaL_checknumber(L, t); break;
        case 'F':   cp->f = (float) luaL_checknumber(L, t); break;
        case 'D':   cp->d = luaL_checknumber(L, t); break;
        case 'S':   /* fall through */
        case 'B': {
            size_t n;
            cp->s = luaL_checklstring(L, t, &n);
            cp->x.y.i = n;
            break;
        }
        case 'V':   cp->v = checkview(L, t); break;
        default:    assert(0);
    }
    return char2type(c);
}

static void parseargs(lua_State *L, vqCell *buf, const char *desc) {
    int i;
    for (i = 0; *desc; ++i)
        check_cell(L, i+1, *desc++, buf+i);
}

#define LVQ_ARGS(state,args,desc) \
            vqCell args[sizeof(desc)-1]; \
            parseargs(state, args, desc)

static vqView table2view (lua_State *L, int t) {
    vqCell cell;
    vqView m, v;
    int r, rows, c, cols;
    lua_getfield(L, t, "meta");
    m = lua_isnil(L, -1) ? desc2meta(L, ":I", 2) : checkview(L, -1);
    lua_pop(L, 1);
    cols = vwRows(m);
    rows = cols > 0 ? lua_objlen(L, t) / cols : 0;
    v = vq_new(m, rows);
    for (c = 0; c < cols; ++c) {
        vqType type = vq_getInt(m, c, 1, VQ_nil) & VQ_TYPEMASK;
        for (r = 0; r < rows; ++r) {
            vqType ty = type;
            lua_pushinteger(L, r * cols + c + 1);
            lua_gettable(L, t);
            ty = lua_isnil(L, -1) ? VQ_nil
                                  : check_cell(L, -1, VQ_TYPES[type], &cell);
            vq_set(v, r, c, ty, cell);
            lua_pop(L, 1);
        }
    }
    return v;
}

static void struct_helper (luaL_Buffer *B, vqView meta) {
    char buf[30];
    int c;
    for (c = 0; c < vwRows(meta); ++c) {
        int type = vq_getInt(meta, c, 1, VQ_nil);
        if ((type & VQ_TYPEMASK) == VQ_view) {
            vqView m = vq_getView(meta, c, 2, 0);
            if (m != 0) {
                if (m == meta)
                    luaL_addchar(B, '^');
                else {
                    luaL_addchar(B, '(');
                    struct_helper(B, m);
                    luaL_addchar(B, ')');
                }
                continue;
            }
        }
        luaL_addstring(B, type2string(type, buf));
    }
}
static void push_struct (vqView v) {
    luaL_Buffer b;
    luaL_buffinit(vwState(v), &b);
    struct_helper(&b, vwMeta(v));
    luaL_pushresult(&b);
}

static void IndirectCleaner (void *p) {
    vqView v = p;
    vq_decref(vwMeta(v));
}
static vqView IndirectView (const vqDispatch *vtabp, vqView meta, int rows, int extra) {
    int c, nc = vwRows(meta);
    vqView v = alloc_view(vtabp, meta, rows, extra);
    for (c = 0; c < nc; ++c) {
        vwCol(v,c).v = v;
        vwCol(v,c).x.y.i = c;
    }
    return v;
}

static void RowColMapCleaner (void *p) {
    vqView v = p, *aux = vwAuxP(v);
    vq_decref(aux[0]);
    vq_decref(aux[1]);
    IndirectCleaner(v);
}
static vqType RowMapGetter (int row, vqCell *cp) {
    int n;
    vqView v = cp->v;
    vqCell *aux = vwAuxP(v);
    vqCell map = aux[1];
    if (map.v != NULL && getcell(row, &map) == VQ_int) {
        /* extra logic to support special maps with relative offsets < 0 */
        if (map.i < 0) {
            row += map.i;
            map = aux[1];
            getcell(row, &map);
        }
        row = map.i;
    }
    v = aux[0].v;
    n = vwRows(v);
    if (row >= n && n > 0)
        row %= n;
    *cp = vwCol(v,cp->x.y.i);
    return getcell(row, cp);
}
static vqDispatch rowmaptab = {
    "rowmap", sizeof(struct vqView_s), 0, RowColMapCleaner, RowMapGetter
};
static vqView RowMapVop (vqView v, vqView map) {
    vqView t, m = vwMeta(map);
    vqCell *aux;
    t = IndirectView(&rowmaptab, vwMeta(v), vwRows(map), 2 * sizeof(vqCell));
    aux = vwAuxP(t);
    aux[0].v = vq_incref(v);
    if (vwRows(m) > 0 && (vq_getInt(m, 0, 1, VQ_nil) & VQ_TYPEMASK) == VQ_int) {
        aux[1] = vwCol(map,0);
        vq_incref(aux[1].v);
    }
    return t;
}
static vqType ColMapGetter (int row, vqCell *cp) {
    vqView v = cp->v;
    int c = cp->x.y.i, nc;
    vqCell *aux = vwAuxP(v);
    vqCell map = aux[1];
    if (map.v != NULL && getcell(c, &map) == VQ_int)
        c = map.i;
    v = aux[0].v;
    nc = vwCols(v);
    if (c >= nc && nc > 0)
        c %= nc;
    *cp = vwCol(v,c);
    return getcell(row, cp);
}
static vqDispatch colmaptab = {
    "colmap", sizeof(struct vqView_s), 0, RowColMapCleaner, ColMapGetter
};
static vqView ColMapVop (vqView v, vqView map) {
    /* TODO: better - store col refs in a new view, avoids extra getter layer */
    vqView t, m = vwMeta(map), mm = RowMapVop(vwMeta(v), map);
    vqCell *aux;
    t = IndirectView(&colmaptab, mm, vwRows(v), 2 * sizeof(vqCell));
    aux = vwAuxP(t);
    aux[0].v = vq_incref(v);
    if (vwRows(m) > 0 && (vq_getInt(m, 0, 1, VQ_nil) & VQ_TYPEMASK) == VQ_int) {
        aux[1] = vwCol(map,0);
        vq_incref(aux[1].v);
    }
    return t;
}

static int colbyname (vqView meta, const char* s) {
    int c, nc = vwRows(meta);
    /* TODO: optimize this dumb linear search */
    for (c = 0; c < nc; ++c)
        if (strcmp(s, vq_getString(meta, c, 0, "")) == 0)
            return c;
    return luaL_error(vwState(meta), "column '%s' not found", s);
}

static int row_call (lua_State *L) {
    vqCell *cp = lua_touserdata(L, 1);
    return push_view(cp->v);
}
static int row_gc (lua_State *L) {
    vqCell *cp = checkrow(L, 1);
    vq_decref(cp->v);
    return 0;
}
static int rowcolcheck (lua_State *L, vqView *pv, int *pr) {
    vqView v, meta;
    int r, c, nc;
    vqCell *cp = checkrow(L, 1);
    *pv = v = cp->v;
    *pr = r = cp->x.y.i;
    meta = vwMeta(v);
    nc = vwRows(meta);
    if (r < 0 || r >= vwRows(v))
        return luaL_error(L, "row index %d out of range", r);
    if (lua_isnumber(L, 2)) {
        c = lua_tointeger(L, 2);
        if (c < 0 || c >= nc)
            return luaL_error(L, "column index %d out of range", c);
    } else
        return colbyname(meta, luaL_checkstring(L, 2));
    return c;
}
static int row_len (lua_State *L) {
    vqCell *cp = lua_touserdata(L, 1);
    lua_pushinteger(L, cp->x.y.i);
    return 1;
}
static int row_index (lua_State *L) {
    vqView v;
    int r, c = rowcolcheck(L, &v, &r);
    vqCell cell = vwCol(v,c);
    vqType type = getcell(r, &cell);
    return pushcell(L, VQ_TYPES[type], &cell);
}
static int row_newindex (lua_State *L) {
    vqView v;
    vqCell cell;
    vqType type = VQ_nil;
    int r, c = rowcolcheck(L, &v, &r);
    if (!lua_isnil(L, 3)) {
        type = vq_getInt(vwMeta(v), c, 1, VQ_nil) & VQ_TYPEMASK;
        type = check_cell(L, 3, VQ_TYPES[type], &cell);
    }
    vq_set(v, r, c, type, cell);
    return 0;
}
static int row2string (lua_State *L) {
    vqCell *cp = checkrow(L, 1);
    lua_pushfstring(L, "row: %d %s ", cp->x.y.i, vDisp(cp->v)->name);
    push_struct(cp->v);
    lua_concat(L, 2);
    return 1;
}

static int view_div (lua_State *L) {
    int c;
    vqView v = checkview(L, 1), w;
    if (lua_isnumber(L, 2))
        c = lua_tonumber(L, 2);
    else if (lua_isstring(L, 2))
        c = colbyname(vwMeta(v), lua_tostring(L, 2));
    else
        return push_view(ColMapVop(v, checkview(L, 2)));
    w = vq_new(desc2meta(L, ":I", 2), 1);
    vq_setInt(w, 0, 0, c);
    return push_view(ColMapVop(v, w));
}
static int view_gc (lua_State *L) {
    vqView v = checkview(L, 1);
    vq_decref(v);
    return 0;
}
static int view_index (lua_State *L) {
    if (lua_isnumber(L, 2)) {
        vqCell *cp;
        LVQ_ARGS(L,A,"VI");
        cp = tagged_udata(L, sizeof *cp, "lvq.row");
        cp->v = vq_incref(A[0].v);
        cp->x.y.i = A[1].i;
    } else if (lua_isstring(L, 2)) {
        const char* s = luaL_checkstring(L, 2);
        lua_getglobal(L, "vops");
        lua_getfield(L, -1, s);
        if (lua_isnil(L, -1))
            return luaL_error(L, "unknown view operator: %s", s);
    } else {
        LVQ_ARGS(L,A,"VV");
        return push_view(RowMapVop(A[0].v, A[1].v));
    }
    return 1;
}
static int view_len (lua_State *L) {
    LVQ_ARGS(L,A,"V");
    lua_pushinteger(L, vwRows(A[0].v));
    return 1;
}
static int view2string (lua_State *L) {
    vqView v;
    LVQ_ARGS(L,A,"V");
    v = A[0].v;
    lua_pushfstring(L, "view: %s #%d ", vDisp(v)->name, vwRows(v));
    push_struct(v);
    lua_concat(L, 2);
    return 1;
}

/* cast all vop arguments to the proper type, then call the real vop */
static int vop_check (lua_State *L) {
    int i;
    const char *fmt = luaL_checkstring(L, lua_upvalueindex(1));
    lua_pushvalue(L, lua_upvalueindex(2));
    for (i = 0; fmt[i]; ++i)
        if (fmt[i] == '-')
            lua_pushvalue(L, i+1);
        else {
            vqCell cell;
            vqType type = check_cell(L, i+1, fmt[i], &cell);
            pushcell(L, VQ_TYPES[type], &cell);
        }
    lua_call(L, i, 1);
    return 1;
}
/* define a vop as a C closure which first casts its args to the proper type */
static int lvq_vopdef (lua_State *L) {
    assert(lua_gettop(L) == 3);
    lua_getglobal(L, "vops");           /* a1 a2 a3 vops */
    lua_insert(L, 1);                   /* vops a1 a2 a3 */
    lua_pushcclosure(L, vop_check, 2);  /* vops a1 f */
    lua_settable(L, 1);                 /* vops */
    return 0;
}

#include "vops.c"
#include "ranges.c"

static const struct luaL_reg lvqlib_row_m[] = {
    {"__call", row_call},
    {"__gc", row_gc},
    {"__index", row_index},
    {"__len", row_len},
    {"__newindex", row_newindex},
    {"__tostring", row2string},
    {0, 0},
};
static const struct luaL_reg lvqlib_view_m[] = {
    {"__div", view_div},
    {"__gc", view_gc},
    {"__index", view_index},
    {"__len", view_len},
    {"__tostring", view2string},
    {0, 0},
};
static const struct luaL_reg lvqlib_f[] = {
    {"vopdef", lvq_vopdef},
    {0, 0},
};

LUA_API int luaopen_lvq_core (lua_State *L) {
    lua_newtable(L); /* t */
    lua_pushstring(L, "v"); /* t s */
    lua_setfield(L, -2, "__mode"); /* t */
    lua_setfield(L, LUA_REGISTRYINDEX, "lvq.pool"); /* <> */

    luaL_newmetatable(L, "lvq.row");
    luaL_register(L, 0, lvqlib_row_m);
    
    luaL_newmetatable(L, "lvq.view");
    luaL_register(L, 0, lvqlib_view_m);
    
    init_empty(L);
    
    lua_newtable(L);
    /* register_vops(L, f_vdispatch); */
    luaL_register(L, NULL, lvqlib_vops);
    luaL_register(L, NULL, lvqlib_ranges);
    lua_setglobal(L, "vops");
    
    luaL_register(L, "lvq", lvqlib_f);
    lua_pushliteral(L, VQ_COPYRIGHT);
    lua_setfield(L, -2, "_COPYRIGHT");
    lua_pushliteral(L, "lvq " VQ_RELEASE);
    lua_setfield(L, -2, "_RELEASE");
    lua_pushliteral(L, "LuaVlerq " VQ_VERSION);
    lua_setfield(L, -2, "_VERSION");
    return 1;
}
