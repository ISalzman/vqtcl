/*  Vlerq base implementation.
    $Id$
    This file is part of Vlerq, see lvq/vlerq.h for full copyright notice. */

#include "vlerq.h"
#include "vqbase.h"

#include <unistd.h>

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
    char *data;
    int off = vtab->prefix;

    data = (char*) lua_newuserdata(L, bytes + off) + off; /* ud */
    PushPool(L); /* ud t */
    lua_pushlightuserdata(L, data); /* ud t key */
    lua_pushvalue(L, -3); /* ud t key ud */
    lua_rawset(L, -3); /* ud t */
    lua_pop(L, 1); /* ud */

    vDisp(data) = vtab;
    return data;
}

static vqView EmptyMetaView (lua_State *L) {
    lua_pushnil(L); /* ... */
    lua_setfield(L, LUA_REGISTRYINDEX, "lvq.emv"); /* <none> */
    return 0;
}

vqView vq_init (lua_State *L) {
    vqView v;
    
    lua_newtable(L); /* t */
    lua_pushstring(L, "v"); /* t s */
    lua_setfield(L, -2, "__mode"); /* t */
    lua_setfield(L, LUA_REGISTRYINDEX, "lvq.pool"); /* <none> */

    v = EmptyMetaView(L);
    vwState(v) = L;
    return v;
}

static void NewDataVec (lua_State *L, vqType type, int rows, vqCell *cp) {
    if (rows == 0) {
        cp->p = 0;
        lua_pushnil(L);
    } else {
        /* ... */
    }
    cp->x.y.i = luaL_ref(L, LUA_REGISTRYINDEX);
}

static void ViewCleaner (vqVec p) {
    vqView v = (vqView) p;
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
    vwRows(v) = rows;
    vwMeta(v) = meta;
    PushView(meta); /* vw ud */
    vHead(v,mref) = luaL_ref(L, LUA_REGISTRYINDEX); /* vw */

    for (c = 0; c < cols; ++c)
        NewDataVec(L, VQ_int, rows, &vwCol(v,c));
    
    return v;
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

