/* tvq.c - Tcl binding for Vlerq */

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

DLLEXPORT int Tvq_Init (Tcl_Interp *interp) {
    if (!MyInitStubs(interp) || Tcl_PkgRequire(interp, "Tcl", "8.4", 0) == NULL)
        return TCL_ERROR;

    Tcl_CreateObjCommand(interp, "tvq", TvqObjCmd, NULL, NULL);
    return Tcl_PkgProvide(interp, "tvq", PACKAGE_VERSION);
}
