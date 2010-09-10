/* V9 C extension for Tcl
   2009-05-08 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
   $Id$ */

#include <tcl.h>
#include "v9.h"
#include "v9x.h"
#include <stdlib.h>
#include <string.h>

#define MAX_ARGS 10

static int CastArgs (const char* name, const char* args, Tcl_Interp* ip, int oc, Tcl_Obj *const ov[], V9Item stack[]); // forward

static void ViewCheck (Tcl_Obj* o) {
    V9View v = VIEWOBJ_VIEW(o);
    // non-view, or string rep, or has deps, or is 0-col view, or is meta-view
    assert(o->typePtr != &V9vType || o->bytes != 0 || VIEWOBJ_DEPS(o) != 0 ||
           V9_Size(V9_Meta(v)) == 0 || V9_Meta(v) == V9_Meta(V9_Meta(v)));
}

static void FreeV9vIntRep (Tcl_Obj *obj) {
    //TODO should Miguel's deletion flag be hardcoded or use TclObjBeingDeleted?
    // see  http://sourceforge.net/tracker/?
    //          func=detail&aid=1512138&group_id=10894&atid=110894
    InvalidateViewObj(obj, obj->length < 0);
    // the deletion flag has a clear effect on performance
}

static void DupV9vIntRep (Tcl_Obj *obj, Tcl_Obj *dup) {
	V9View v = VIEWOBJ_VIEW(obj);
    assert(v != 0);
	V9_AddRef(v);
	VIEWOBJ_VIEW(dup) = v;
    VIEWOBJ_DEPS(dup) = 0;
	dup->typePtr = &V9vType;
	
	CreateDeps(dup, VIEWOBJ_DEPS(obj)->argv);

    ViewCheck(dup);
}

static void UpdateV9vStrRep (Tcl_Obj *obj) {
	V9View v = VIEWOBJ_VIEW(obj);
	if (V9_Size(V9_Meta(v)) == 0) {
	    // zero-column views can easily be converted to a string
        char buf[20];
        sprintf(buf, "%d", V9_Size(v));
        obj->length = strlen(buf);
		obj->bytes = ckalloc(obj->length + 1);
		strcpy(obj->bytes, buf);
	} else if (V9_Meta(v) == V9_Meta(V9_Meta(v))) {
		// meta-views can also be converted to a string on-demand
		obj->length = V9_MetaAsDescLength(v);
		obj->bytes = ckalloc(obj->length + 1);
		V9_MetaAsDesc(v, obj->bytes);
	} else {
		// otherwise we need a list rep to generate the string
        ViewDeps* vdp = VIEWOBJ_DEPS(obj);
    	if (vdp != 0) {
    		const char* s = Tcl_GetStringFromObj(vdp->argv, &obj->length);
    		obj->bytes = ckalloc(obj->length + 1);
    		strcpy(obj->bytes, s);
    	} else
    		Tcl_Panic("UpdateV9vStrRep?");
	}
}

static int SetV9vFromAnyRep (Tcl_Interp* ip, Tcl_Obj *obj) {
    int oc;
    Tcl_Obj** ov;    
    if (Tcl_ListObjGetElements(ip, obj, &oc, &ov) != TCL_OK)
        return TCL_ERROR;

	V9View v = 0;
	if (oc == 0)
		v = V9_Release(V9_AddRef(0)); // weird trick to get the empty meta-view
	else if (oc == 1) {
		int rows;
        if (Tcl_GetIntFromObj(ip, ov[0], &rows) == TCL_OK) {
			if (rows < 0)
				Tcl_SetResult(ip, "can't use value < 0 as view", TCL_STATIC);
			else
				v = V9_NewDataView(0, rows);
		} else
            v = V9_DescAsMeta(Tcl_GetString(ov[0]), 0);
	} else {
		int index;
	    if (Tcl_GetIndexFromObjStruct(ip, ov[0], HandlerTable,
				sizeof *HandlerTable, "handler", TCL_EXACT, &index) != TCL_OK)
			return TCL_ERROR;
        ViewHandler* vh = HandlerTable + index;
        V9Item stack[MAX_ARGS];
        if (CastArgs(vh->name, vh->args, ip, oc, ov, stack) != TCL_OK)
            return TCL_ERROR;
		v = vh->proc(ip, stack);
	}
	if (v == 0)
		return TCL_ERROR;
	
	// prepare a copy of this object, now a list, if we set up dependencies
    Tcl_Obj* list = oc > 1 ? Tcl_DuplicateObj(obj) : 0;

	if (obj->typePtr != 0 && obj->typePtr->freeIntRepProc != 0)
    	obj->typePtr->freeIntRepProc(obj);

    VIEWOBJ_VIEW(obj) = V9_AddRef(v);
    VIEWOBJ_DEPS(obj) = 0;
    obj->typePtr = &V9vType;

    if (list != 0)
        CreateDeps(obj, list);

    ViewCheck(obj);
	return TCL_OK;
}

Tcl_ObjType V9vType = {
	"v9v", FreeV9vIntRep, DupV9vIntRep, UpdateV9vStrRep, SetV9vFromAnyRep
};

V9View ObjAsView (Tcl_Interp* ip, Tcl_Obj* obj) {
    if (Tcl_ConvertToType(ip, obj, &V9vType) != TCL_OK)
        return 0;
	return VIEWOBJ_VIEW(obj);
}

int ObjAsItem (Tcl_Interp* ip, Tcl_Obj* obj, V9Types type, V9Item* pitem) {
	switch (type) {
        case V9T_nil:
            assert(0);
			return 0;
		case V9T_int:
			return Tcl_GetIntFromObj(ip, obj, &pitem->i) == TCL_OK;
		case V9T_long:
			return Tcl_GetWideIntFromObj(ip, obj,
											(Tcl_WideInt*) &pitem->l) == TCL_OK;
		case V9T_float:
			if (Tcl_GetDoubleFromObj(ip, obj, &pitem->d) != TCL_OK)
				return 0;
			pitem->f = (float) pitem->d;
			break;
		case V9T_double:
			return Tcl_GetDoubleFromObj(ip, obj, &pitem->d) == TCL_OK;
		case V9T_string:
			pitem->s = Tcl_GetString(obj);
			break;
		case V9T_bytes:
			pitem->p = Tcl_GetByteArrayFromObj(obj, &pitem->c.i);
			break;
		case V9T_view:
			pitem->v = ObjAsView(ip, obj);
			return pitem->v != 0;
	}
	return 1;
}

Tcl_Obj* ViewAsHalfObj (V9View v) {
	Tcl_Obj* obj = Tcl_NewObj();
    VIEWOBJ_VIEW(obj) = V9_AddRef(v);
	VIEWOBJ_DEPS(obj) = 0;
    obj->typePtr = &V9vType;
    // result obj *must* be adjusted to have a string or list rep before handing
    // it back to Tcl, see "if (type == V9T_view ..." in operatorCmd()
	Tcl_InvalidateStringRep(obj);
	return obj;
}

Tcl_Obj* ItemAsObj (Tcl_Interp* ip, V9Types type, V9Item item) {
	switch (type) {
		case V9T_nil:
			return Tcl_NewObj();
		case V9T_int:
			return Tcl_NewIntObj(item.i);
		case V9T_long:
			return Tcl_NewWideIntObj(item.l);
		case V9T_float:
			return Tcl_NewDoubleObj(item.f);
		case V9T_double:
			return Tcl_NewDoubleObj(item.d);
		case V9T_string:
			return Tcl_NewStringObj(item.s, -1);
		case V9T_bytes:
			return Tcl_NewByteArrayObj(item.p, item.c.i);
		case V9T_view:
			return ViewAsHalfObj(item.v); // incomplete
	}
	assert(0);
	return 0;
}

//FIXME can't use a Buffer here, use a more primitive static int vec for now
Storage UnwindSubviews (Tcl_Interp* ip, Tcl_Obj* obj, int* buf) {
    int b = 0;
    V9View v;
    
    for (;;) {
        v = ObjAsView(ip, obj);
        if (v == 0)
            return 0;
        
        ViewDeps* vdp = VIEWOBJ_DEPS(obj);
        if (vdp == 0)
            break;

        int oc;
        Tcl_Obj** ov;
        if (Tcl_ListObjGetElements(ip, vdp->argv, &oc, &ov) != TCL_OK)
            return 0;

        if (oc != 4 || strcmp(Tcl_GetString(ov[0]), "at") != 0)
            break; // it's not a subview operation

        if (Tcl_GetIntFromObj(ip, ov[2], buf + b++) != TCL_OK ||
                Tcl_GetIntFromObj(ip, ov[3], buf + b++) != TCL_OK)
            return 0;

        obj = ov[1];
    }
    buf[b] = ~0;
    
    const char* str = Tcl_GetString(obj);
    if (strncmp(str, "vref ", 5) == 0) {
        //FIXME must access as list, in case storage name has list escapes
        Storage sp = GetStorageInfo(ip, str + 5);
        if (sp != 0)
            return sp;
        Tcl_ResetResult(ip);
    }
    
    Tcl_AppendResult(ip, "not a storage view: ", str, 0);
    return 0;
}

static int CastArgs (const char* name, const char* args, Tcl_Interp* ip, int oc, Tcl_Obj *const ov[], V9Item stack[]) {
    assert(strlen(args) <= MAX_ARGS);
    --oc; ++ov;
    int i;
    for (i = 0; args[i] != 0; ++i) {
        if (args[i] == 'X') {
            stack[i].c.p = (void*) (ov + i);
            stack[i].c.i = oc - i;
            break;
        }
        if ((args[i] == 0 && i != oc) || (args[i] != 0 && i >= oc)) {
            char buf [10*MAX_ARGS];
            const char* s;
            *buf = 0;
            for (i = 0; args[i] != 0; ++i) {
                if (*buf != 0)
                    strcat(buf, " ");
                switch (args[i] & ~0x20) {
                    case 'O': s = "any"; break;
                    case 'I': s = "int"; break;
                    case 'S': s = "string"; break;
                    case 'B': s = "bytes"; break;
                    case 'V': s = "view"; break;
                    case 'X': s = "..."; break;
                    default: s = "?"; break;
                }
                strcat(buf, s);
                if (args[i] & 0x20)
                    strcat(buf, "*");
            }
            Tcl_WrongNumArgs(ip, 1, ov - 1, buf);
            return TCL_ERROR;
        }
        switch (args[i]) {
            case 'O':
                stack[i].p = ov[i];
                break;
            default:
                if (!ObjAsItem(ip, ov[i], V9_CharAsType(args[i]), stack+i)) {
                    if (*Tcl_GetStringResult(ip) == 0) {
                        const char* s = "";
                        switch (args[i] & ~0x20) {
                            case 'I': s = "integer "; break;
                            case 'V': s = "view "; break;
                        }
                        Tcl_AppendResult(ip, name, ": invalid ", s, "arg", 0);
                    }
                    return TCL_ERROR;
                }
        }
    }
    
    return TCL_OK;
}

static int operatorCmd (ClientData cd, Tcl_Interp* ip, int oc, Tcl_Obj *const ov[]) {
    ViewOperator* op = cd;
    assert(op->types[1] == ':');
    V9Item stack [MAX_ARGS];
    if (CastArgs(op->name, op->types+2, ip, oc, ov, stack) != TCL_OK)
        return TCL_ERROR;
        
    V9Types type = op->proc(ip, stack);
    if (type == V9T_nil)
        return TCL_ERROR;

    Tcl_Obj* obj;
    switch (op->types[0]) {
        case 'O':   obj = ov[1]; break; // original object
        case 'P':   obj = stack[0].p; break; // pass object
        default:    obj = ItemAsObj(ip, type, stack[0]); break;
    }
    if (obj == 0)
        return TCL_ERROR;

    // save cmd and args if this is a view result with no string and no list rep
    //TODO potential problem: meta-view with str rep no longer tracks deps?
    if (type == V9T_view && obj->bytes == 0 && VIEWOBJ_DEPS(obj) == 0) {
        Tcl_Obj* list = Tcl_NewListObj(oc, ov);
        // use op->name as cmd, not ov[0] which is fully-qualified with ::'s
        Tcl_Obj* oname = Tcl_NewStringObj(op->name, -1);
        Tcl_ListObjReplace(0, list, 0, 1, 1, &oname);
        CreateDeps(obj, list);
    }

    ViewCheck(obj);
    Tcl_SetObjResult(ip, obj);
    return TCL_OK;
}

EXTERN int V9x_Init (Tcl_Interp* interp) {
    if (Tcl_InitStubs(interp, "8.1", 0) == 0 ||
         Tcl_PkgRequire(interp, "Tcl", "8.4", 0) == 0 ||
          Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION) != TCL_OK)
	    return TCL_ERROR;

	char buf[30];
	strcpy(buf, "v9::");
	ViewOperator* vop;
	for (vop = OperatorTable; vop->name != 0; ++vop) {
		strcpy(buf + 4, vop->name);
	    Tcl_CreateObjCommand(interp, buf, operatorCmd, vop, 0);
	}

    return TCL_OK;
}
