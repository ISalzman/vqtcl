/*
 * ext_tcl.h - Definitions needed across the Tcl-specific code.
 */

#include <stdlib.h>
#include <string.h>

#include "../src/intern.h"
#include "../src/wrap_gen.h"

#define USE_TCL_STUBS 1
#include <tcl.h>

/* colobj.c */

extern Tcl_ObjType f_colObjType;

Object_p  (ColumnAsObj) (Column column);

/* ext_tcl.c */

typedef struct SharedInfo {
    Tcl_Interp *interp;
    const struct CmdDispatch *cmds;
} SharedInfo;

#define Interp() (GetShared()->info->interp)
#define DecObjRef   Tcl_DecrRefCount

Object_p  (BufferAsTclList) (Buffer_p bp);
Object_p  (IncObjRef) (Object_p objPtr);

/* viewobj.c */

extern Tcl_ObjType f_viewObjType;

int       (AdjustCmdDef) (Object_p cmd);
Object_p  (ViewAsObj) (View_p view);
