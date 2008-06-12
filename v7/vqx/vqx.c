/*  Vlerq standalone executable utility.
    $Id$
    This file is part of Vlerq, see src/vlerq.h for full copyright notice. */

#include "luall.c"
#include "vq.c"

const char vqx_ident[] = "$Vlerq: " VQ_RELEASE " " VQ_COPYRIGHT " $\n";

int main (int argc, char **argv) {
    int i;
    vqEnv env = luaL_newstate();
    luaL_openlibs(env);

    lua_getglobal(env, "package");                /* p */
    lua_getfield(env, -1, "loaded");              /* p l */
    
    lua_pushcfunction(env, luaopen_vq_core);      /* p l f */
    lua_call(env, 0, 1);                          /* p l m */
    
    lua_setfield(env, -2, "vq.core");             /* p l */
    lua_pop(env, 2);                              /* <> */
    
    lua_createtable(env, 0, argc);                /* t */
    for (i = 1; i < argc; i++) {
      lua_pushstring(env, argv[i]);               /* t s */
      lua_rawseti(env, -2, i);                    /* t */
    }
    lua_setglobal(env, "arg");                    /* <> */
    
    if (luaL_dofile(env, "vqx.init")) {
        fprintf(stderr, "vqx: %s\n", lua_tostring(env, -1));
        return 1;
    }
        
    lua_close(env);
    return 0;
}
