/* V9 C extension for Tcl - view operators
   2009-05-08 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
   $Id$ */

#include <tcl.h>
#include "v9.h"
#include "v9x.h"
#include <string.h>

static V9Types metaOp_V (Tcl_Interp* ip, V9Item args[]) {
	args->v = V9_Meta(args[0].v);
    return V9T_view;
}

static V9Types sizeOp_V (Tcl_Interp* ip, V9Item args[]) {
	args->i = V9_Size(args[0].v);
    return V9T_int;
}

static V9Types atOp_VII (Tcl_Interp* ip, V9Item args[]) {
    V9View v = args[0].v;
    *args = V9_Get(v, args[1].i, args[2].i, 0);
	return V9_GetInt(V9_Meta(v), args[2].i, 1);
}

static V9Types vallocOp_V (Tcl_Interp* ip, V9Item args[]) {
	args->s = V9_AllocName(args[0].v);
    return V9T_string;
}

static V9Types vtypeOp_V (Tcl_Interp* ip, V9Item args[]) {
	args->s = V9_TypeName(args[0].v);
    return V9T_string;
}

static V9Types storageOp_VS (Tcl_Interp* ip, V9Item args[]) {
    args->p = CreateStorage(ip, args[0].v, args[1].s);
    return V9T_string;
}

static V9Types atsetOp_OIIO (Tcl_Interp* ip, V9Item args[]) {
    V9View v = ObjAsView(ip, args[0].p);

    int ivec[10]; //FIXME use a dynamic mechanism, i.e. V9's internal Buffer
    ivec[0] = args[1].i;
    ivec[1] = args[2].i;

    Storage sp = UnwindSubviews(ip, args[0].p, ivec + 2);
	if (sp == 0)
		return V9T_nil;

    V9Types type = V9_GetInt(V9_Meta(v), ivec[1], 1);
	V9Item item;
	if (!ObjAsItem(ip, args[3].p, type, &item))
		return V9T_nil;
		
    if (!ModifyStorage(sp, ivec, item, 0)) {
        //XXX never happens
        Tcl_SetResult(ip, "failed to set item", TCL_STATIC);
        return V9T_nil;
    }

    return V9T_view;
}

static V9Types replaceOp_OIIV (Tcl_Interp* ip, V9Item args[]) {
    int ivec[10]; //FIXME use a dynamic mechanism, i.e. V9's internal Buffer
    ivec[0] = args[1].i;
    ivec[1] = args[2].i;

    Storage sp = UnwindSubviews(ip, args[0].p, ivec + 2);
	if (sp == 0)
		return V9T_nil;

    if (!ModifyStorage(sp, ivec, args[3], 1)) {
        //XXX never happens
        Tcl_SetResult(ip, "failed to replace row(s)", TCL_STATIC);
        return V9T_nil;
    }

    return V9T_view;
}

static void* EmitInitFun (void* aux, intptr_t len) {
    return Tcl_SetByteArrayLength(aux, len);
}

static void* EmitDataFun (void *aux, const void *ptr, intptr_t len) {
    return (char*) memcpy(aux, ptr, len) + len;
}

static V9Types emitOp_V (Tcl_Interp* ip, V9Item args[]) {
    Tcl_Obj* obj = Tcl_NewObj();
    intptr_t n = V9_ViewEmit(args[0].v, obj, EmitInitFun, EmitDataFun);
    args->p = obj;
    return n > 0 ? V9T_bytes : V9T_nil;
}

static V9Types roweqOp_VIVI (Tcl_Interp* ip, V9Item args[]) {
    args->i = V9_RowEqual(args[0].v, args[1].i, args[2].v, args[3].i);
    return V9T_int;
}

static V9Types rowcmpOp_VIVI (Tcl_Interp* ip, V9Item args[]) {
    args->i = V9_RowCompare(args[0].v, args[1].i, args[2].v, args[3].i);
    return V9T_int;
}

static V9Types vcmpOp_VV (Tcl_Interp* ip, V9Item args[]) {
    args->i = V9_ViewCompare(args[0].v, args[1].v);
    return V9T_int;
}

ViewOperator OperatorTable[] = {
    { "at",      "U:VII",  atOp_VII       },
    { "atset",   "O:OIIO", atsetOp_OIIO   },
    { "emit",    "P:V",    emitOp_V       },
    { "meta",    "V:V",    metaOp_V       },
    { "replace", "O:OIIV", replaceOp_OIIV },
    { "rowcmp",  "I:VIVI", rowcmpOp_VIVI  },
    { "roweq",   "I:VIVI", roweqOp_VIVI   },
    { "size",    "I:V",    sizeOp_V       },
    { "storage", "P:VS",   storageOp_VS   },
    { "valloc",  "S:V",    vallocOp_V     },
    { "vcmp",    "I:VV",   vcmpOp_VV      },
    { "vtype",   "S:V",    vtypeOp_V      },
	{ 0, 0 }
};
