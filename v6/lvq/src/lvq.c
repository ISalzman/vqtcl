#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "lvq.h"

#include "vcore.c"

static int view_gc (lua_State *L) {
    vq_View v = *(vq_View*) lua_touserdata(L, 1);
    if (v != NULL) puts("view: gc");
    return 0;
}

static vq_View checkview (lua_State *L, int index) {
    void *ud = luaL_checkudata(L, index, "LuaVlerq.view");
    luaL_argcheck(L, ud != NULL, index, "'view' expected");
    return *(vq_View*) ud;
}

static int view_meta (lua_State *L) {
    vq_View v = checkview(L, 1);
    return 1;
}

static int view_size (lua_State *L) {
    vq_View v = checkview(L, 1);
    return 1;
}

static int view2string (lua_State *L) {
    vq_View v = checkview(L, 1);
    lua_pushfstring(L, "view[%d]:ISV", vq_size(v));
    return 1;
}

static int lvq_new (lua_State *L) {
    vq_View *metap = (vq_View*) lua_touserdata(L, 1);
    int size = luaL_checkint(L, 2);
    vq_View *viewp = (vq_View*) lua_newuserdata(L, sizeof(vq_View));
    luaL_getmetatable(L, "LuaVlerq.view");
    lua_setmetatable(L, -2);
    *viewp = vq_new(*metap, size);
    return 1;
}

static const struct luaL_reg vqlib_m[] = {
    {"__tostring", view2string},
    {"meta", view_meta},
    {"size", view_size},
    {NULL, NULL},
};

static const struct luaL_reg vqlib_f[] = {
    {"new", lvq_new},
    {NULL, NULL},
};

static void set_info (lua_State *L) {
    lua_pushliteral (L, "_COPYRIGHT");
    lua_pushliteral (L, "Copyright (C) 2007 Jean-Claude Wippler");
    lua_settable (L, -3);
    lua_pushliteral (L, "_DESCRIPTION");
    lua_pushliteral (L, "Lua Views and Queries");
    lua_settable (L, -3);
    lua_pushliteral (L, "_VERSION");
    lua_pushliteral (L, "LuaVlerq 1.6.0");
    lua_settable (L, -3);
}

int luaopen_lvq_core (lua_State *L) {
    luaL_newmetatable(L, "LuaVlerq.view");
    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, -3);
    lua_pushstring(L, "__gc");
    lua_pushcfunction(L, view_gc);
    lua_settable(L, -3);
    luaL_register(L, NULL, vqlib_m);
    luaL_register(L, "lvq", vqlib_f);
    set_info (L);
    return 1;
}
