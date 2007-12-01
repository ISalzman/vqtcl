/* tvq.c - Tcl binding for Vlerq */

#if defined(VQ_EMBED_LUA)

#define luaall_c

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

#endif

#include "lvq.c"

#define USE_TCL_STUBS 1
#include <tcl.h>

/* stub interface code, removes the need to link with libtclstub*.a */
#ifdef STATIC_BUILD
#define MyInitStubs(x) 1
#else
#include "stubs.h"
#endif

static int TvqObjCmd (ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
    Tcl_AppendResult(interp, "bla bla", NULL);
    return TCL_OK;
}

static int LuaObjCmd (ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
    if (objc > 1)
        luaL_dostring(data, Tcl_GetString(objv[1]));
    Tcl_AppendResult(interp, "bla bla", NULL);
    return TCL_OK;
}

static void LuaDelProc (ClientData data) {
    lua_close(data);    
}

DLLEXPORT int Tvq_Init (Tcl_Interp *interp) {
    lua_State *L;
    
    if (!MyInitStubs(interp) || Tcl_PkgRequire(interp, "Tcl", "8.4", 0) == NULL)
        return TCL_ERROR;

    L = lua_open();
    Tcl_CreateExitHandler(LuaDelProc, L);
    
    luaL_openlibs(L);

    lua_pushcfunction(L, luaopen_lvq_core);
    lua_pushstring(L, "lvq.core");
    lua_call(L, 1, 0);
    luaL_dostring(L, "package.loaded['lvq.core'] = lvq");
    
    Tcl_CreateObjCommand(interp, "lua", LuaObjCmd, L, NULL);
    Tcl_CreateObjCommand(interp, "tvq", TvqObjCmd, L, NULL);
    return Tcl_PkgProvide(interp, "tvq", PACKAGE_VERSION);
}
