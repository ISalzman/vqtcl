/*  Lua extension binding.
    This file is part of LuaVlerq.
    See lvq.h for full copyright notice.
    $Id$  */

#include "lua.h"
#include "lauxlib.h"
#include "lvq.h"
#include "vcore.c"

#include <string.h>

static vq_Item * checkrow (lua_State *L, int idx) {
    void *ud = luaL_checkudata(L, idx, "LuaVlerq.row");
    luaL_argcheck(L, ud != NULL, idx, "'row' expected");
    return ud;
}

static vq_View checkview (lua_State *L, int idx) {
    void *ud = luaL_checkudata(L, idx, "LuaVlerq.view");
    luaL_argcheck(L, ud != NULL, idx, "'view' expected");
    return *(vq_View*) ud;
}

static int row_call (lua_State *L) {
    vq_Item *vi = checkrow(L, 1);
    int c = luaL_checkint(L, 2);
    lua_pushinteger(L, vi->o.b.i * 1000 + c);
    return 1;
}

static int row_gc (lua_State *L) {
    vq_Item vi = *(vq_Item*) lua_touserdata(L, 1);
    puts("row: gc");
    return 0;
}

static int row_index (lua_State *L) {
    vq_Item *vi = checkrow(L, 1);
    const char *c = luaL_checkstring(L, 2);
    lua_pushinteger(L, strlen(c));
    return 1;
}

static int row2string (lua_State *L) {
    vq_Item *vi = checkrow(L, 1);
    lua_pushfstring(L, "row[%d]", vi->o.b.i);
    return 1;
}

static const struct luaL_reg vqlib_row_m[] = {
    {"__call", row_call},
    {"__gc", row_gc},
    {"__index", row_index},
    {"__tostring", row2string},
    {NULL, NULL},
};

static int view_meta (lua_State *L) {
    vq_View v = checkview(L, 1);
    vq_meta(v);
    return 1;
}

static int view_gc (lua_State *L) {
    vq_View v = *(vq_View*) lua_touserdata(L, 1);
    if (v != NULL) puts("view: gc");
    return 0;
}

static int view_index (lua_State *L) {
    vq_View v = checkview(L, 1);
    int r = luaL_checkint(L, 2);
    vq_Item *rowp = (vq_Item*) lua_newuserdata(L, sizeof(vq_Item));
    luaL_getmetatable(L, "LuaVlerq.row");
    lua_setmetatable(L, -2);
    rowp->o.a.m = v;
    rowp->o.b.i = r;
    return 1;
}

static int view_len (lua_State *L) {
    vq_View v = checkview(L, 1);
    lua_pushinteger(L, vq_size(v));
    return 1;
}

static int view2string (lua_State *L) {
    vq_View v = checkview(L, 1);
    lua_pushfstring(L, "view[%d]:ISV", vq_size(v));
    return 1;
}

static const struct luaL_reg vqlib_view_m[] = {
    {"meta", view_meta},
    {"__gc", view_gc},
    {"__index", view_index},
    {"__len", view_len},
    {"__tostring", view2string},
    {NULL, NULL},
};

static int lvq_view (lua_State *L) {
    vq_View *metap = (vq_View*) lua_touserdata(L, 1);
    int size = luaL_checkint(L, 2);
    vq_View *viewp = (vq_View*) lua_newuserdata(L, sizeof(vq_View));
    luaL_getmetatable(L, "LuaVlerq.view");
    lua_setmetatable(L, -2);
    *viewp = vq_new(metap ? *metap : NULL, size);
    return 1;
}

static const struct luaL_reg vqlib_f[] = {
    {"view", lvq_view},
    {NULL, NULL},
};

static void set_info (lua_State *L) {
    lua_pushliteral (L, "_COPYRIGHT");
    lua_pushliteral (L, VQ_COPYRIGHT);
    lua_settable (L, -3);
    lua_pushliteral (L, "_VERSION");
    lua_pushliteral (L, "LuaVlerq " VQ_VERSION);
    lua_settable (L, -3);
}

int luaopen_lvq_core (lua_State *L) {
    luaL_newmetatable(L, "LuaVlerq.row");
    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, -3);
    luaL_register(L, NULL, vqlib_row_m);
    
    luaL_newmetatable(L, "LuaVlerq.view");
    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, -3);
    luaL_register(L, NULL, vqlib_view_m);
    
    luaL_register(L, "lvq", vqlib_f);
    set_info (L);    
    return 1;
}
