/*  Vlerq standalone executable.
    $Id$
    This file is part of Vlerq, see base/vlerq.h for full copyright notice. */

#define VQ_EMBED_LUA

#include "lvq.c"

int main (int argc, char **argv) {
    lua_State *L = lua_open();
    luaL_openlibs(L);
    
    lua_pushcfunction(L, luaopen_lvq_core);
    lua_pushstring(L, "lvq.core");
    lua_call(L, 1, 0);
    
    if (argc > 1 && luaL_dofile(L, argv[1]) != 0) {
        fprintf(stderr,"%s\n", lua_tostring(L, -1));
        return 1;
    }
    
    lua_close(L);
    return 0;
}
