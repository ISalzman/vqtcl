/*  Lua extension binding.
    $Id$
    This file is part of Vlerq, see lvq/vlerq.h for full copyright notice. */

#include <lauxlib.h>

#include "vqbase.c"

/*
static int view_rows (lua_State *L) {
    vqView v = check_view(L, 1);
    lua_pushinteger(L, vwRows(v));
    return 1;
}
*/

static const struct luaL_reg lvqlib_f[] = {
    {NULL, NULL},
};

LUA_API int luaopen_lvq_core (lua_State *L) {
    luaL_register(L, "lvq", lvqlib_f);
    lua_pushliteral(L, "LuaVlerq " VQ_VERSION);
    lua_setfield(L, -2, "_VERSION");
    lua_pushliteral(L, VQ_COPYRIGHT);
    lua_setfield(L, -2, "_COPYRIGHT");
    return 1;
}
