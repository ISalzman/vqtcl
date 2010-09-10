/* V9 C extension for Tcl - dependency handling
   2009-05-08 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
   $Id$ */

#include <tcl.h>
#include "v9.h"
#include "v9x.h"

static void AddAsDrain (Tcl_Obj* obj, Tcl_Obj* me) {
    ViewDeps* vdp = VIEWOBJ_DEPS(obj);
    if (vdp != 0) {
        // add me as drain to vdp
        if (vdp->deps >= vdp->limit) {
            int newlimit = (vdp->limit * 3) / 2 + 3;
            int bytes = sizeof *vdp + newlimit * sizeof (Tcl_Obj*);
            vdp = VIEWOBJ_DEPS(obj) = (void*) ckrealloc((void*) vdp, bytes);
            vdp->limit = newlimit;
        }
        vdp->drains[vdp->deps++] = me;

        VIEWOBJ_DEPS(me)->sources++;
    }
}

static void RemoveAsDrain (Tcl_Obj* obj, Tcl_Obj* me) {
    ViewDeps* vdp = VIEWOBJ_DEPS(obj);
    if (vdp != 0) {
        // remove me as drain from vdp
        int i;
        for (i = 0; i < vdp->deps; ++i)
            if (vdp->drains[i] == me)
                break;
        assert(i < vdp->deps);
        vdp->drains[i] = vdp->drains[--(vdp->deps)];
        
        VIEWOBJ_DEPS(me)->sources--;
    }
}

void CreateDeps (Tcl_Obj* obj, Tcl_Obj* list) {
    ViewDeps* vdp = (void*) ckalloc(sizeof *vdp);
    vdp->deps = vdp->limit = vdp->sources = 0;
    vdp->argv = list;
    Tcl_IncrRefCount(vdp->argv);
    VIEWOBJ_DEPS(obj) = vdp;

    int i, oc;
    Tcl_Obj** ov;
    Tcl_ListObjGetElements(0, vdp->argv, &oc, &ov);
    for (i = 1; i < oc; ++i)
        if (IS_VIEWOBJ(ov[i]))
            AddAsDrain(ov[i], obj);
}

void InvalidateDrains (Tcl_Obj* obj) {
    ViewDeps* vdp = VIEWOBJ_DEPS(obj);
    assert(vdp != 0);

    while (vdp->deps > 0) {
        int j = vdp->deps - 1;
        // if it's listed as drain here, then it must have a ViewDeps
        assert(VIEWOBJ_DEPS(vdp->drains[j]) != 0);
        InvalidateViewObj(vdp->drains[j], 0);
        assert(vdp->deps == j);
    }
    
    assert(vdp->deps == 0);
}

void InvalidateViewObj (Tcl_Obj* obj, int deleting) {
    ViewDeps* vdp = VIEWOBJ_DEPS(obj);
	if (vdp != 0) {
        InvalidateDrains(obj);

        int i, oc;
        Tcl_Obj** ov;
        Tcl_ListObjGetElements(0, vdp->argv, &oc, &ov);
        for (i = 1; i < oc; ++i)
            if (IS_VIEWOBJ(ov[i]))
                RemoveAsDrain(ov[i], obj);

        assert(vdp->sources == 0);

        // make sure there is a string rep, now that the list is about to go
        if (!deleting && obj->bytes == 0)
            Tcl_GetString(obj);
        
	    Tcl_DecrRefCount(vdp->argv);
        ckfree((char*) vdp);
	}
        
    V9View v = VIEWOBJ_VIEW(obj);
	V9_Release(v);
	
    obj->typePtr = 0;
}
