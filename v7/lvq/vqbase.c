/*  Vlerq base implementation.
    $Id$
    This file is part of Vlerq, see lvq/vlerq.h for full copyright notice. */

#include "vlerq.h"
#include "vqbase.h"

#include <unistd.h>

static void get_pool (lua_State *L) {
    lua_getfield(L, LUA_REGISTRYINDEX, "lvq.pool");
}

static void get_view (vqView v) {
    lua_State *L = vwState(v);
    get_pool(L); /* t */
    lua_pushlightuserdata(L, v); /* t key */
    lua_rawget(L, -2); /* t ud */
    lua_remove(L, -2); /* ud */
}

static void *AllocVector (lua_State *L, const vqDispatch *vtab, int bytes) {
    char *data;
    int off = vtab->prefix * sizeof(vqCell);

    data = (char*) lua_newuserdata(L, bytes + off) + off; /* ud */
    get_pool(L); /* ud t */
    lua_pushlightuserdata(L, data); /* ud t key */
    lua_pushvalue(L, -3); /* ud t key ud */
    lua_rawset(L, -3); /* ud t */
    lua_pop(L, 1); /* ud */

    vDisp(data) = vtab;
    return data;
}

static vqType NotYetGetter (int row, vqCell *cp) {
    VQ_UNUSED(row);
    VQ_UNUSED(cp);
    return VQ_nil;
}

static vqView EmptyMetaView (void) {
    return 0;
}

vqView vq_init (lua_State *L) {
    vqView v;
    
    lua_newtable(L);
    lua_pushstring(L, "v");
    lua_setfield(L, -2, "__mode");
    lua_setfield(L, LUA_REGISTRYINDEX, "lvq.pool");

    v = EmptyMetaView();
    vwState(v) = L;
    return v;
}

lua_State *vq_state (vqView v) {
    return vwState(v);
}

static vqDispatch vtab = { "?", 0, 0, 0, NotYetGetter };

vqView vq_new (vqView m, int rows) {
    lua_State *L = vwState(m);
    
    vqView v = AllocVector(L, &vtab, vwRows(m) * sizeof(vqCell));
    vwRows(v) = rows;
    vwMeta(v) = m;
    
    lua_newtable(L);
    get_view(m);
    lua_setfield(L, -2, "meta");
    lua_setfenv(L, -2);
    
    return v;
}

vqView vq_meta (vqView v) {
    return vwMeta(v);
}

int vq_rows (vqView v) {
    return vwRows(v);
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
    vDisp(v)->setter(v, row, col, type != VQ_nil ? &val : 0);
    return v;
}

vqView vq_replace (vqView v, int start, int count, vqView data) {
    vDisp(v)->replacer(v, start, count, data);
    return v;
}

