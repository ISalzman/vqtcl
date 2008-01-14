/*  Lua extension binding.
    $Id$
    This file is part of Vlerq, see lvq/vlerq.h for full copyright notice. */

#include <lauxlib.h>

#include "vlerq.h"
#include "vqbase.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static vqView check_view (lua_State *L, int t); /* forward */
    
static vqView EmptyMeta (lua_State *L) {
    vqView v;
    lua_getfield(L, LUA_GLOBALSINDEX, "lvq_emv"); /* t */
    v = check_view(L, -1);
    lua_pop(L, 1);
    return v;
}

static int CharAsType (char c) {
    const char *p = strchr(VQ_TYPES, c & ~0x20);
    int type = p != 0 ? p - VQ_TYPES : VQ_nil;
    if (c & 0x20)
        type |= VQ_NULLABLE;
    return type;
}

/*
static int StringAsType (const char *str) {
    int type = CharAsType(*str);
    while (*++str != 0)
        if ('a' <= *str && *str <= 'z')
        type |= 1 << (*str - 'a' + 5);
    return type;
}
*/

static const char* TypeAsString (int type, char *buf) {
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

static vqView ParseDesc (vqView emv, char **desc, const char **nbuf, int *tbuf, vqView *sbuf) {
    char sep, *ptr = *desc;
    const char  **names = nbuf;
    int c, cols = 0, *types = tbuf;
    vqView result, *subts = sbuf;
    
    for (;;) {
        const char* s = ptr;
        sbuf[cols] = emv;
        tbuf[cols] = VQ_string;
        
        while (strchr(":,[]", *ptr) == 0)
            ++ptr;
        sep = *ptr;
        *ptr = 0;

        if (sep == '[') {
            ++ptr;
            sbuf[cols] = ParseDesc(emv, &ptr, nbuf+cols, tbuf+cols, sbuf+cols);
            tbuf[cols] = VQ_view;
            sep = *++ptr;
        } else if (sep == ':') {
            tbuf[cols] = CharAsType(*++ptr);
            sep = *++ptr;
        }
        
        nbuf[cols++] = s;
        if (sep != ',')
            break;
            
        ++ptr;
    }
    
    *desc = ptr;

    result = vq_new(vwMeta(emv), cols);

    for (c = 0; c < cols; ++c)
        vq_setMetaRow(result, c, names[c], types[c], subts[c]);
    
    return result;
}

static vqView DescLenToMeta (lua_State *L, const char *desc, int length) {
    int i, bytes, limit = 1;
    void *buffer;
    const char **nbuf;
    int *tbuf;
    vqView *sbuf, meta;
    char *dbuf;
    vqView emv = EmptyMeta(L);
    
    if (length <= 0)
        return emv;
    
    /* find a hard upper bound for the buffer requirements */
    for (i = 0; i < length; ++i)
        if (desc[i] == ',' || desc[i] == '[')
            ++limit;
    
    bytes = limit * (2 * sizeof(void*) + sizeof(int)) + length + 1;
    buffer = malloc(bytes);
    nbuf = buffer;
    sbuf = (void*) (nbuf + limit);
    tbuf = (void*) (sbuf + limit);
    dbuf = memcpy(tbuf + limit, desc, length);
    dbuf[length] = ']';
    
    meta = ParseDesc(emv, &dbuf, nbuf, tbuf, sbuf);
    
    free(buffer);
    return meta;
}

static vqView AsMetaVop (lua_State *L, const char *desc) {
    return DescLenToMeta(L, desc, strlen(desc));
}

static void PushPool (lua_State *L) {
    lua_getfield(L, LUA_GLOBALSINDEX, "lvq_pool");
}

static int PushView (vqView v) {
    lua_State *L = vwState(v);
    PushPool(L); /* t */
    lua_pushlightuserdata(L, v); /* t key */
    lua_rawget(L, -2); /* t ud */
    lua_remove(L, -2); /* ud */
    return 1;
}

static void *PushNewVector (lua_State *L, const vqDispatch *vtab, int bytes) {
    char *data = lua_newuserdata(L, bytes + vtab->prefix); /* ud */
    memset(data, 0, bytes + vtab->prefix);
    data += vtab->prefix;
    
    PushPool(L); /* ud t */
    lua_pushlightuserdata(L, data); /* ud t key */
    lua_pushvalue(L, -3); /* ud t key ud */
    lua_rawset(L, -3); /* ud t */
    lua_pop(L, 1); /* ud */

    vDisp(data) = vtab;
    return data;
}

static vqType NilVecGetter (int row, vqCell *cp) {
    const char *p = cp->p;
    return (p[row/8] >> (row&7)) & 1 ? VQ_int : VQ_nil;
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
    vqCell *p = q;
    VQ_UNUSED(col);
/* FIXME: crashes
    if (p[row].v != 0)
        luaL_unref(vwState(p[row].v), LUA_REGISTRYINDEX, p[row].x.y.i);
*/
    p[row].v = cp->v;
    if (cp->v != 0)
        p[row].x.y.i = luaL_ref(vwState(cp->v), LUA_REGISTRYINDEX);
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
    lua_State *L = 0;
    vqCell *p = q;
    for (i = 0; i < vSize(q); ++i)
        if (p[i].v != 0) {
            if (L == 0)
                L = vwState(p[i].v);
            luaL_unref(L, LUA_REGISTRYINDEX, p[i].x.y.i);
        }
}

static vqDispatch nvtab = {
    "nilvec", 2, -1, 0, NilVecGetter, NilVecSetter
};
static vqDispatch ivtab = {
    "intvec", 2, 4, 0, IntVecGetter, IntVecSetter
};
static vqDispatch lvtab = {
    "longvec", 2, 8, 0, LongVecGetter, LongVecSetter 
};
static vqDispatch fvtab = {
    "floatvec", 2, 4, 0, FloatVecGetter, FloatVecSetter 
};
static vqDispatch dvtab = {
    "doublevec", 2, 8, 0, DoubleVecGetter, DoubleVecSetter 
};
static vqDispatch svtab = {
    "stringvec", 2, sizeof(void*), 
    StringVecCleaner, StringVecGetter, StringVecSetter
};
static vqDispatch bvtab = {
    "bytesvec", 2, sizeof(vqCell), 
    BytesVecCleaner, BytesVecGetter, BytesVecSetter
};
static vqDispatch tvtab = {
    "viewvec", 2, sizeof(void*), 
    ViewVecCleaner, ViewVecGetter, ViewVecSetter
};

static void NewDataVec (lua_State *L, vqType type, int rows, vqCell *cp) {
    if (rows == 0) {
        cp->p = 0;
        lua_pushnil(L);
    } else {
        int bytes;
        vqDispatch *vtab;
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
                vtab = &tvtab; bytes = rows * sizeof(vqView); break;
            default: assert(0); return;
        }
        cp->v = PushNewVector(L, vtab, bytes);
        vSize(cp->p) = /* vLimit(cp->p) = */ rows;
    }
    cp->x.y.i = luaL_ref(L, LUA_REGISTRYINDEX);
}

static void ViewCleaner (void *p) {
    vqView v = p;
    lua_State *L = vwState(v);
    int c, cols = vwCols(v);
    for (c = 0; c < cols; ++c)
        luaL_unref(L, LUA_REGISTRYINDEX, vwCol(v,c).x.y.i);
    luaL_unref(L, LUA_REGISTRYINDEX, vHead(v,mref));
}

static vqDispatch vtab = { "view", sizeof(struct vqView_s), 0, ViewCleaner };

vqView vq_new (vqView meta, int rows) {
    vqView v;
    lua_State *L = vwState(meta);
    int c, cols = vwRows(meta);
    
    v = PushNewVector(L, &vtab, cols * sizeof(vqCell)); /* vw */
    vwState(v) = L;
    vwRows(v) = rows;
    vwMeta(v) = meta;
    PushView(meta); /* vw ud */
    vHead(v,mref) = luaL_ref(L, LUA_REGISTRYINDEX); /* vw */

    for (c = 0; c < cols; ++c)
        NewDataVec(L, VQ_int, rows, &vwCol(v,c));
    
    luaL_getmetatable(L, "lvq.view"); /* ud mt */
    lua_setmetatable(L, -2); /* ud */

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
    vq_setView(m, row, 2, sub != 0 ? sub : EmptyMeta(vwState(m)));
}

static void InitEmpty (lua_State *L) {
    vqView meta, mm;
    
    lua_newtable(L); /* t */
    lua_pushstring(L, "v"); /* t s */
    lua_setfield(L, -2, "__mode"); /* t */
    lua_setfield(L, LUA_GLOBALSINDEX, "lvq_pool"); /* <> */

    mm = PushNewVector(L, &vtab, 3 * sizeof *mm); /* vw */
    luaL_getmetatable(L, "lvq.view"); /* vw mt */
    lua_setmetatable(L, -2); /* vw */
    vwState(mm) = L;
    vwRows(mm) = 3;
    vwMeta(mm) = mm;
    vHead(mm,mref) = luaL_ref(L, LUA_REGISTRYINDEX); /* <> */
    NewDataVec(L, VQ_string, 3, &vwCol(mm,0));
    NewDataVec(L, VQ_int, 3, &vwCol(mm,1));
    NewDataVec(L, VQ_view, 3, &vwCol(mm,2));
    
    meta = vq_new(mm, 0); /* vw */
    lua_setfield(L, LUA_GLOBALSINDEX, "lvq_emv"); /* <> */

    vq_setMetaRow(mm, 0, "name", VQ_string, meta);
    vq_setMetaRow(mm, 1, "type", VQ_int, meta);
    vq_setMetaRow(mm, 2, "subv", VQ_view, meta);
}

static vqType GetCell (int row, vqCell *cp) {
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
    return GetCell(row, &cell) == VQ_nil;
}

vqCell vq_get (vqView v, int row, int col, vqType type, vqCell def) {
    vqCell cell;
    VQ_UNUSED(type); /* TODO: check type arg */
    if (row < 0 || row >= vwRows(v) || col < 0 || col >= vwCols(v))
        return def;
    cell = vwCol(v, col);
    return GetCell(row, &cell) == VQ_nil ? def : cell;
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

static vqRow *checkrow (lua_State *L, int t) {
    return luaL_checkudata(L, t, "lvq.row");
}

static vqView check_view (lua_State *L, int t) {
    vqView v;

    switch (lua_type(L, t)) {
        case LUA_TNIL:      v = EmptyMeta(L);
                            break;
        case LUA_TBOOLEAN:  v = vq_new(EmptyMeta(L), lua_toboolean(L, t));
                            break;
        case LUA_TNUMBER:   v = vq_new(EmptyMeta(L), lua_tointeger(L, t));
                            break;
        case LUA_TSTRING:   v = AsMetaVop(L, lua_tostring(L, t));
                            break;
#if 0
        case LUA_TTABLE:    v = TableToView(L, t);
                            break;
#endif
        default:            v = luaL_checkudata(L, t, "lvq.view");
                            return v+1; /* views point past their header */
    }
    
    lua_replace(L, t);
    return v;
}

static int pushcell (lua_State *L, char c, vqCell *cp) {
    if (cp == 0)
        return 0;
        
    switch (c) {
        case 'N':   lua_pushnil(L); break;
        case 'I':   lua_pushinteger(L, cp->i); break;
        case 'L':   lua_pushnumber(L, cp->l); break;
        case 'F':   lua_pushnumber(L, cp->f); break;
        case 'D':   lua_pushnumber(L, cp->d); break;
        case 'S':   lua_pushstring(L, cp->s); break;
        case 'B':   lua_pushlstring(L, cp->p, cp->x.y.i); break;
        case 'V':   PushView(cp->v); break;
    /* pseudo-types */
        case 'T':   lua_pushboolean(L, cp->i); break;
        default:    assert(0);
    }

    return 1;
}

static vqType checkitem (lua_State *L, int t, char c, vqCell *cp) {
    size_t n;
    vqType type;

    if ('a' <= c && c <= 'z') {
        if (lua_isnoneornil(L, t)) {
            cp->p = cp->x.y.p = 0;
            return VQ_nil;
        }
        c += 'A'-'a';
    }
    
    type = CharAsType(c);

    switch (c) {
        case 'N':   break;
        case 'I':   cp->i = luaL_checkinteger(L, t); break;
        case 'L':   cp->l = (int64_t) luaL_checknumber(L, t); break;
        case 'F':   cp->f = (float) luaL_checknumber(L, t); break;
        case 'D':   cp->d = luaL_checknumber(L, t); break;
        case 'S':   /* fall through */
        case 'B':   cp->s = luaL_checklstring(L, t, &n);
                    cp->x.y.i = n; break;
        case 'V':   cp->v = check_view(L, t); break;
        default:    assert(0);
    }

    return type;
}

static void parseargs(lua_State *L, vqCell *buf, const char *desc) {
    int i;
    for (i = 0; *desc; ++i)
        checkitem(L, i+1, *desc++, buf+i);
}

#define LVQ_ARGS(state,args,desc) \
            vqCell args[sizeof(desc)-1]; \
            parseargs(state, args, desc)

static int row_gc (lua_State *L) {
    vqRow *rp = checkrow(L, 1);
    luaL_unref(vwState(rp->view), LUA_REGISTRYINDEX, rp->ref);
    return 0;
}

static int rowcolcheck (lua_State *L, vqView *pv, int *pr) {
    vqView v, meta;
    int r, c, cols;
    vqRow *rp = checkrow(L, 1);
    *pv = v = rp->view;
    *pr = r = rp->row;
    meta = vwMeta(v);
    cols = vwRows(meta);
    if (r < 0 || r >= vwRows(v))
        return luaL_error(L, "row index %d out of range", r);
    if (lua_isnumber(L, 2)) {
        c = lua_tointeger(L, 2);
        if (c < 0 || c >= cols)
            return luaL_error(L, "column index %d out of range", c);
    } else {
        const char *s = luaL_checkstring(L, 2);
        /* TODO: optimize this dumb linear search */
        for (c = 0; c < cols; ++c)
            if (strcmp(s, vq_getString(meta, c, 0, "")) == 0)
                return c;
        return luaL_error(L, "column '%s' not found", s);
    }   
    return c;
}

static int row_index (lua_State *L) {
    vqView v;
    int r, c = rowcolcheck(L, &v, &r);
    vqCell cell = vwCol(v,c);
    return pushcell(L, VQ_TYPES[GetCell(r, &cell)], &cell);
}

static int row_newindex (lua_State *L) {
    vqView v;
    vqCell cell;
    vqType type = VQ_nil;
    int r, c = rowcolcheck(L, &v, &r);
    if (!lua_isnil(L, 3)) {
        type = vq_getInt(vwMeta(v), c, 1, VQ_nil) & VQ_TYPEMASK;
        type = checkitem(L, 3, VQ_TYPES[type], &cell);
    }
    vq_set(v, r, c, type, cell);
    return 0;
}

static int view_gc (lua_State *L) {
    vqView v = lua_touserdata(L, 1); assert(v != 0);
/* FIXME: crashes
    vDisp(v)->cleaner(v);
*/
    return 0;
}

static int view_index (lua_State *L) {
    if (lua_isnumber(L, 2)) {
        vqRow *rp;
        LVQ_ARGS(L,A,"VI");
        rp = lua_newuserdata(L, sizeof *rp);
        luaL_getmetatable(L, "lvq.row"); /* ud mt */
        lua_setmetatable(L, -2); /* ud */
        rp->view = A[0].v;
        rp->row = A[1].i;
        PushView(A[0].v);
        rp->ref = luaL_ref(L, LUA_REGISTRYINDEX);
    } else {
        const char* s = luaL_checkstring(L, 2);
        lua_getglobal(L, "vops");
        lua_getfield(L, -1, s);
        if (lua_isnil(L, -1))
            return luaL_error(L, "unknown '%s' view operator", s);
    }
    return 1;
}

static int view_len (lua_State *L) {
    LVQ_ARGS(L,A,"V");
    lua_pushinteger(L, vwRows(A[0].v));
    return 1;
}

static void view2struct (luaL_Buffer *B, vqView meta) {
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
                    view2struct(B, m);
                    luaL_addchar(B, ')');
                }
                continue;
            }
        }
        luaL_addstring(B, TypeAsString(type, buf));
    }
}

static void PushStruct (vqView v) {
    luaL_Buffer b;
    luaL_buffinit(vwState(v), &b);
    view2struct(&b, vwMeta(v));
    luaL_pushresult(&b);
}

static int view2string (lua_State *L) {
    vqView v;
    LVQ_ARGS(L,A,"V");
    v = A[0].v;
    lua_pushfstring(L, "view: %s #%d ", vDisp(v)->name, vwRows(v));
    PushStruct(v);
    lua_concat(L, 2);
    return 1;
}

static int row2string (lua_State *L) {
    vqRow *rp = checkrow(L, 1);
    lua_pushfstring(L, "row: %d %s ", rp->row, vDisp(rp->view)->name);
    PushStruct(rp->view);
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
            vqType type = checkitem(L, i+1, fmt[i], &cell);
            pushcell(L, VQ_TYPES[type], &cell);
        }
    lua_call(L, i, 1);
    return 1;
}

static int vops_meta (lua_State *L) {
    LVQ_ARGS(L,A,"V");
    return PushView(vwMeta(A[0].v));
}

static int vops_view (lua_State *L) {
    LVQ_ARGS(L,A,"Vv");
    if (A[1].v != 0)
        A->v = vq_new(A[1].v, vwRows(A[0].v));
    return PushView(A->v);
}

static int lvq_emv (lua_State *L) {
    return PushView(EmptyMeta(L));
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

static const struct luaL_reg lvqlib_row_m[] = {
    {"__gc", row_gc},
    {"__index", row_index},
    {"__newindex", row_newindex},
    {"__tostring", row2string},
    {0, 0},
};

static const struct luaL_reg lvqlib_view_m[] = {
    {"__gc", view_gc},
    {"__index", view_index},
    {"__len", view_len},
    {"__tostring", view2string},
    {0, 0},
};

static const struct luaL_reg lvqlib_vops[] = {
    {"meta", vops_meta},
    {"view", vops_view},
    {0, 0},
};

static const struct luaL_reg lvqlib_f[] = {
    {"emv", lvq_emv},
    {"vopdef", lvq_vopdef},
    {0, 0},
};

LUA_API int luaopen_lvq_core (lua_State *L) {
    luaL_newmetatable(L, "lvq.row");
    luaL_register(L, 0, lvqlib_row_m);
    
    luaL_newmetatable(L, "lvq.view");
    luaL_register(L, 0, lvqlib_view_m);
    
    InitEmpty(L);
    
    lua_newtable(L);
    /* register_vops(L, f_vdispatch); */
    luaL_register(L, NULL, lvqlib_vops);
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
