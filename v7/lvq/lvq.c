/*  Lua extension binding.
    $Id$
    This file is part of Vlerq, see lvq/vlerq.h for full copyright notice. */

#include "vlerq.h"

#include <lauxlib.h>

static const struct luaL_reg lvqlib_f[] = {
    {NULL, NULL},
};

LUA_API int luaopen_lvq_core (lua_State *L) {

    luaL_register(L, "lvq", lvqlib_f);
    
    lua_pushliteral(L, VQ_COPYRIGHT);
    lua_setfield(L, -2, "_COPYRIGHT");
    lua_pushliteral(L, "LuaVlerq " VQ_VERSION);
    lua_setfield(L, -2, "_VERSION");

    return 1;
}
