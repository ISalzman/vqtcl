/* V9 C extension for Tcl - private interface
   2009-05-08 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
   $Id$ */

#include <assert.h>

#define IS_VIEWOBJ(o)   ((o)->typePtr == &V9vType)
#define VIEWOBJ_VIEW(o) (*((V9View*)&(o)->internalRep.twoPtrValue.ptr1))
#define VIEWOBJ_DEPS(o) (*((ViewDeps**)&(o)->internalRep.twoPtrValue.ptr2))

typedef struct ViewDeps {
    Tcl_Obj* argv;
    int deps;
    int limit;
    int sources;
    Tcl_Obj* drains[]; // must be last member
} ViewDeps;

typedef struct ViewHandler {
    const char* name;
    const char* args;
    V9View (*proc)(Tcl_Interp*,V9Item[]);
} ViewHandler;

typedef struct ViewOperator {
    const char* name;
    const char* types;
    V9Types (*proc)(Tcl_Interp*,V9Item[]);
} ViewOperator;

typedef struct StorageInfo* Storage;

// v9x.c
extern Tcl_ObjType V9vType;

V9View ObjAsView (Tcl_Interp* ip, Tcl_Obj* obj);
int ObjAsItem (Tcl_Interp* ip, Tcl_Obj* obj, V9Types type, V9Item* pitem);
Tcl_Obj* ViewAsHalfObj (V9View v);
Tcl_Obj* ItemAsObj (Tcl_Interp* ip, V9Types type, V9Item item);
Storage UnwindSubviews (Tcl_Interp* ip, Tcl_Obj* obj, int* buf);

// v9x_depend.c
void CreateDeps (Tcl_Obj* obj, Tcl_Obj* list);
void InvalidateDrains (Tcl_Obj* obj);
void InvalidateViewObj (Tcl_Obj* obj, int deleting);

// v9x_handler.c
extern ViewHandler HandlerTable[];

// v9x_operator.c
extern ViewOperator OperatorTable[];

// v9x_storage.c
Tcl_Obj* CreateStorage (Tcl_Interp* ip, V9View v, const char* cmd);
Storage GetStorageInfo (Tcl_Interp* ip, const char* cname);
V9View GetViewFromStorage (Storage sp);
int ModifyStorage (Storage sp, const int* ivec, V9Item item, int op);
