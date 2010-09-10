/* V9 C extension for Tcl - view handlers
   2009-05-08 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
   $Id$ */

#include <tcl.h>
#include "v9.h"
#include "v9x.h"

static V9View atHandler_VII (Tcl_Interp* ip, V9Item args[]) {
	return V9_GetView(args[0].v, args[1].i, args[2].i);
}

static V9View vdatHandler_VX (Tcl_Interp* ip, V9Item args[]) {
	V9View v = 0, meta = args[0].v;
    assert(V9_Meta(meta) == V9_Meta(V9_Meta(meta)));
	
    Tcl_Obj** objv = args[1].c.p;
    int objc = args[1].c.i;
    
	int nc = V9_Size(meta);
	if (nc == 0 || objc != nc)
		goto FAIL1;
	int nr;
	if (Tcl_ListObjLength(ip, objv[0], &nr) != TCL_OK)
		goto FAIL3;

	v = V9_NewDataView(meta, nr);
    V9_AddRef(v); //FIXME: bug in refcounts, this shouldn't be here
	int c;
	for (c = 0; c < nc; ++c) {
		int ec;
		Tcl_Obj** ev;
	    if (Tcl_ListObjGetElements(ip, objv[c], &ec, &ev) != TCL_OK)
	        goto FAIL2;
		if (ec != nr)
			goto FAIL1;
		int r, type = V9_GetInt(meta, c, 1);
		for (r = 0; r < nr; ++r) {
			V9Item item;
			if (!ObjAsItem(ip, ev[r], type, &item))
				goto FAIL2;
			V9_Set(v, r, c, &item);
		}
	}
	return v;	

FAIL1:
	Tcl_SetResult(ip, "inconsistent element count in vdat", TCL_STATIC);
FAIL2:
	V9_Release(v);
FAIL3:
	V9_Release(meta);
	return 0;
}

static V9View vmapHandler_B (Tcl_Interp* ip, V9Item args[]) {
	return V9_MapDataAsView(args[0].c.p, args[0].c.i);
}

static V9View vrefHandler_S (Tcl_Interp* ip, V9Item args[]) {
    Storage sp = GetStorageInfo(ip, args[0].s);
	return GetViewFromStorage(sp);
}

static V9View stepHandler_IIII (Tcl_Interp* ip, V9Item args[]) {
	return V9_StepView(args[0].i, args[1].i, args[2].i, args[3].i);
}

static V9View rowmapHandler_VV (Tcl_Interp* ip, V9Item args[]) {
	return V9_RowMapView(args[0].v, args[1].v);
}

static V9View colmapHandler_VV (Tcl_Interp* ip, V9Item args[]) {
	return V9_ColMapView(args[0].v, args[1].v);
}

static V9View plusHandler_VV (Tcl_Interp* ip, V9Item args[]) {
	return V9_PlusView(args[0].v, args[1].v);
}

static V9View pairHandler_VV (Tcl_Interp* ip, V9Item args[]) {
	return V9_PairView(args[0].v, args[1].v);
}

static V9View timesHandler_VI (Tcl_Interp* ip, V9Item args[]) {
	return V9_TimesView(args[0].v, args[1].i);
}

static V9View sortmapHandler_V (Tcl_Interp* ip, V9Item args[]) {
	return V9_SortMap(args[0].v);
}

static V9View isectmapHandler_VV (Tcl_Interp* ip, V9Item args[]) {
	return V9_IntersectMap(args[0].v, args[1].v);
}

static V9View exceptmapHandler_VV (Tcl_Interp* ip, V9Item args[]) {
	return V9_ExceptMap(args[0].v, args[1].v);
}

static V9View revisectmapHandler_VV (Tcl_Interp* ip, V9Item args[]) {
	return V9_RevIntersectMap(args[0].v, args[1].v);
}

static V9View uniqmapHandler_V (Tcl_Interp* ip, V9Item args[]) {
	return V9_UniqMap(args[0].v);
}

ViewHandler HandlerTable[] = {
	{ "at",          "VII",  atHandler_VII         },
	{ "colmap",      "VV",   colmapHandler_VV      },
	{ "exceptmap",   "VV",   exceptmapHandler_VV   },
	{ "isectmap",    "VV",   isectmapHandler_VV    },
	{ "pair",        "VV",   pairHandler_VV        },
	{ "plus",        "VV",   plusHandler_VV        },
	{ "revisectmap", "VV",   revisectmapHandler_VV },
	{ "rowmap",      "VV",   rowmapHandler_VV      },
	{ "sortmap",     "V",    sortmapHandler_V      },
	{ "step",        "IIII", stepHandler_IIII      },
	{ "times",       "VI",   timesHandler_VI       },
	{ "uniqmap",     "V",    uniqmapHandler_V      },
	{ "vdat",        "VX",   vdatHandler_VX        },
	{ "vmap",        "B",    vmapHandler_B         },
	{ "vref",        "S",    vrefHandler_S         },
	{ 0, 0 }                                                    
};
