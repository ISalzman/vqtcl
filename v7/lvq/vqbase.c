/*  Vlerq base implementation.
    $Id$
    This file is part of Vlerq, see lvq/vlerq.h for full copyright notice. */

#include "vlerq.h"
#include "vqbase.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static vqView check_view (lua_State *L, int t); /* forward */
    
static vqView EmptyMeta (lua_State *L) {
    vqView v;
    lua_getfield(L, LUA_REGISTRYINDEX, "lvq.emv"); /* t */
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
    lua_getfield(L, LUA_REGISTRYINDEX, "lvq.pool");
}

static void PushView (vqView v) {
    lua_State *L = vwState(v);
    PushPool(L); /* t */
    lua_pushlightuserdata(L, v); /* t key */
    lua_rawget(L, -2); /* t ud */
    lua_remove(L, -2); /* ud */
}

static void *PushNewVector (lua_State *L, const vqDispatch *vtab, int bytes) {
    char *data = lua_newuserdata(L, bytes + vtab->prefix); /* ud */
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
    /* cp->i = (p[row/8] >> (row&7)) & 1; */
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
    vqVec v = q;
    char *p = (char*) v;
    int bit = 1 << (row&7);
    VQ_UNUSED(col);
    if (cp != 0)
        p[row/8] |= bit;
    else
        p[row/8] &= ~bit;
}
static void IntVecSetter (void *q, int row, int col, const vqCell *cp) {
    vqVec v = q;
    int *p = (int*) v;
    VQ_UNUSED(col);
    p[row] = cp != 0 ? cp->i : 0;
}
static void LongVecSetter (void *q, int row, int col, const vqCell *cp) {
    vqVec v = q;
    int64_t *p = (int64_t*) v;
    VQ_UNUSED(col);
    p[row] = cp != 0 ? cp->l : 0;
}
static void FloatVecSetter (void *q, int row, int col, const vqCell *cp) {
    vqVec v = q;
    float *p = (float*) v;
    VQ_UNUSED(col);
    p[row] = cp != 0 ? cp->f : 0;
}
static void DoubleVecSetter (void *q, int row, int col, const vqCell *cp) {
    vqVec v = q;
    double *p = (double*) v;
    VQ_UNUSED(col);
    p[row] = cp != 0 ? cp->d : 0;
}
static void StringVecSetter (void *q, int row, int col, const vqCell *cp) {
    vqVec v = q;
    const char **p = (const char**) v, *x = p[row];
    VQ_UNUSED(col);
    p[row] = cp != 0 ? strcpy(malloc(strlen(cp->s)+1), cp->s) : 0;
    free((void*) x);
}
static void BytesVecSetter (void *q, int row, int col, const vqCell *cp) {
    vqVec v = q;
    vqCell *p = (vqCell*) v, x = p[row];
    VQ_UNUSED(col);
    if (cp != 0) {
        p[row].p = memcpy(malloc(cp->x.y.i), cp->p, cp->x.y.i);
        p[row].x.y.i = cp->x.y.i;
    } else {
        p[row].p = 0;
        p[row].x.y.i = 0;
    }
    free(x.p);
}
static void ViewVecSetter (void *q, int row, int col, const vqCell *cp) {
    lua_State *L = vwState(cp->v);
    vqVec v = q;
    vqCell *p = (vqCell*) v;
    VQ_UNUSED(col);
    luaL_unref(L, LUA_REGISTRYINDEX, p[row].x.y.i);
    p[row].v = cp->v;
    p[row].x.y.i = luaL_ref(L, LUA_REGISTRYINDEX);
}

static void StringVecCleaner (void *q) {
    int i;
    vqView v = q;
    const char **p = (const char**) v;
    for (i = 0; i < vwRows(v); ++i)
        free((void*) p[i]);
}
static void BytesVecCleaner (void *q) {
    int i;
    vqView v = q;
    vqCell *p = q;
    for (i = 0; i < vwRows(v); ++i)
        free(p[i].p);
}
static void ViewVecCleaner (void *q) {
    int i;
    vqView v = q;
    lua_State *L = vwState(v);
    vqCell *p = q;
    for (i = 0; i < vwRows(v); ++i)
        luaL_unref(L, LUA_REGISTRYINDEX, p[i].x.y.i);
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
        vwRows(cp->v) = /* vLimit(cp->v) = */ rows;
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
    lua_setfield(L, LUA_REGISTRYINDEX, "lvq.pool"); /* <> */

    mm = PushNewVector(L, &vtab, 3 * sizeof *mm); /* vw */
    vwState(mm) = L;
    vwRows(mm) = 3;
    vwMeta(mm) = mm;
    vHead(mm,mref) = luaL_ref(L, LUA_REGISTRYINDEX); /* <> */
    NewDataVec(L, VQ_string, 3, &vwCol(mm,0));
    NewDataVec(L, VQ_int, 3, &vwCol(mm,1));
    NewDataVec(L, VQ_view, 3, &vwCol(mm,2));
    
    meta = vq_new(mm, 0); /* vw */
    lua_setfield(L, LUA_REGISTRYINDEX, "lvq.emv"); /* <> */

    vq_setMetaRow(mm, 0, "name", VQ_string, meta);
    vq_setMetaRow(mm, 1, "type", VQ_int, meta);
    vq_setMetaRow(mm, 2, "subv", VQ_view, meta);
}

static vqType GetCell (int row, vqCell *cp) {
    vqVec v = cp->p;
    if (v == 0)
        return VQ_nil;
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

