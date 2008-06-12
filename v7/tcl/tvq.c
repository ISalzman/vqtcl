/*  Vlerq as Tcl extension.
    $Id$
    This file is part of Vlerq, see src/vlerq.h for full copyright notice. */

#include "lualib.h"
#include "vq.c"

#define USE_TCL_STUBS 1
#include <tcl.h>

/* shorthand */
#define _ptr1   internalRep.twoPtrValue.ptr1
#define _ptr2   internalRep.twoPtrValue.ptr2
#define tclAppend(l,e)    Tcl_ListObjAppendElement(NULL,l,e)

/* stub interface code, removes the need to link with libtclstub*.a */
#if defined(STATIC_BUILD)
#define MyInitStubs(x) 1
#else
#include "stubs.h"
#endif

const char vqx_ident[] = "$Vlerq: " VQ_RELEASE " " VQ_COPYRIGHT " $\n";

DLLEXPORT int Tvq_Init (Tcl_Interp *interp) {
    vqEnv env;
    int ipref;
    
    if (!MyInitStubs(interp) || Tcl_PkgRequire(interp, "Tcl", "8.4", 0) == NULL)
        return TCL_ERROR;

    /* set up a new instance on Lua, make it clean up when Tcl finishes */
    env = lua_open();
    Tcl_CreateExitHandler((Tcl_ExitProc*) lua_close, env);
    
    /* first off, store a pointer to the Tcl interpreter as reference #1 */
  	lua_pushlightuserdata(env, interp);
    ipref = luaL_ref(env, LUA_REGISTRYINDEX);
    assert(ipref == 1); /* make sure it really is 1, see TclInterpreter() */

    luaL_openlibs(env); /* TODO: do we need all the standard Lua libs? */

    /* initialize vq */
    lua_pushcfunction(env, luaopen_vq_core);
    lua_call(env, 0, 0);

    return Tcl_PkgProvide(interp, "tvq", VQ_RELEASE);
}
