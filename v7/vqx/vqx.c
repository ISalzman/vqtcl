#define luaall_c

/* this takes care of Windows, Mac OS X, and Linux builds */
#ifndef __WIN32
#define LUA_USE_POSIX 1
#endif

#include "lapi.c"
#include "lcode.c"
#include "ldebug.c"
#include "ldo.c"
#include "ldump.c"
#include "lfunc.c"
#include "lgc.c"
#include "llex.c"
#include "lmem.c"
#include "lobject.c"
#include "lopcodes.c"
#include "lparser.c"
#include "lstate.c"
#include "lstring.c"
#include "ltable.c"
#include "ltm.c"
#include "lundump.c"
#include "lvm.c"
#include "lzio.c"

#include "lauxlib.c"
#include "lbaselib.c"
#include "ldblib.c"
#include "liolib.c"
#include "linit.c"
#include "lmathlib.c"
#include "loadlib.c"
#include "loslib.c"
#include "lstrlib.c"
#include "ltablib.c"

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
