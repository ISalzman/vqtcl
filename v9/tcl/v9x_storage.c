/* V9 C extension for Tcl - mutable storage
   2009-05-08 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
   $Id$ */

#include <tcl.h>
#include "v9.h"
#include "v9x.h"
#include <stdlib.h>
#include <string.h>

struct StorageInfo {
    Tcl_Obj* vref;
};

static int StorageCmd (ClientData cd, Tcl_Interp* ip, int oc, Tcl_Obj *const ov[]) {
    Storage sp = cd;

	if (oc != 1) {
		Tcl_WrongNumArgs(ip, 1, ov, "");
		return TCL_ERROR;
	}

	Tcl_SetObjResult(ip, sp->vref);
    return TCL_OK;
}

static void StorageDelProc (ClientData cd) {
    Storage sp = cd;
    Tcl_DecrRefCount(sp->vref);
    ckfree((char*) sp);
}

Tcl_Obj* CreateStorage (Tcl_Interp* ip, V9View v, const char* cmd) {    
    Storage sp = (Storage) ckalloc(sizeof *sp);
    Tcl_Command token = Tcl_CreateObjCommand(ip, cmd, StorageCmd, sp,
                                                            StorageDelProc);
    Tcl_Obj* vref[2];
    vref[0] = Tcl_NewStringObj("vref", 4);
    vref[1] = Tcl_NewObj();
    Tcl_GetCommandFullName(ip, token, vref[1]);
    Tcl_Obj* list = Tcl_NewListObj(2, vref);
    
    sp->vref = ViewAsHalfObj(v);
	CreateDeps(sp->vref, list); // for setup only, no actual deps
    Tcl_IncrRefCount(sp->vref);

	return sp->vref;
}

Storage GetStorageInfo (Tcl_Interp* ip, const char* cname) {
    Tcl_CmdInfo info;
    if (Tcl_GetCommandInfo(ip, cname, &info) && info.objProc == StorageCmd)
        return info.objClientData;
	
    Tcl_AppendResult(ip, "no such storage object: ", cname, 0);
    return 0;
}

V9View GetViewFromStorage (Storage sp) {
    return VIEWOBJ_VIEW(sp->vref);
}

int ModifyStorage(Storage sp, const int* ivec, V9Item item, int op) {
    //XXX extremely ugly code and fixed stack sizes used for now
    
    // count how many "at" nesting levels are present
    int limit = 0;
    while (ivec[limit] != ~0)
        ++limit;
    
    // descend into the subview, keeping all the intermediate views in a stack
    V9View stack[10]; // will only use elements 2, 4, 6, etc
    int i = limit;
    stack[i] = VIEWOBJ_VIEW(sp->vref);
    while (i > 2) {
        i -= 2;
        stack[i] = V9_AddRef(V9_Get(stack[i+2], ivec[i], ivec[i+1], 0).v);
    }
    
    // now apply the requested change
    V9View v = 0;
    switch (op) {
        case 0: v = V9_Set(stack[2], ivec[0], ivec[1], &item); break;
        case 1: v = V9_Replace(stack[2], ivec[0], ivec[1], item.v); break;
    }
    assert(v != 0);
    
    // ascend back up, replacing subviews along the way
    //XXX need to check whether the view has changed
    while (i < limit) {
        i += 2;
        V9Item temp;
        temp.v = v;
        v = V9_Set(stack[i], ivec[i-2], ivec[i-1], &temp);
        assert(v != 0);
        //FIXME refcounts are not yet right - V9_Release(stack[i]);
    }
    
    // finish by replacing the current storage view, if it's a different one
    if (v != VIEWOBJ_VIEW(sp->vref)) {
        V9_Release(VIEWOBJ_VIEW(sp->vref));
        VIEWOBJ_VIEW(sp->vref) = V9_AddRef(v);
    }
    InvalidateDrains(sp->vref);
    
    return 1;
}
