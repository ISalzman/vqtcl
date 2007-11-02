/* Vlerq public C header */

#ifndef VQ_DEFS_H
#define VQ_DEFS_H

#include "vq4c.h"

#include <assert.h>

#define VQ_NULLABLE (1 << 4)
#define VQ_TYPEMASK (VQ_NULLABLE - 1)

typedef vq_Table Vector; /* every vq_Table is a Vector, but not vice-versa */

#ifdef _TCL
typedef struct Tcl_Obj *Object_p;
#else
typedef void *Object_p;
#endif

#define vType(vecptr)   ((vecptr)[-1].o.a.h)
#define vRefs(vecptr)   ((vecptr)[-1].o.b.i)
#define vMeta(vecptr)   ((vecptr)[-2].o.a.m)
#define vLimit(vecptr)  ((vecptr)[-2].o.a.i)
#define vCount(vecptr)  ((vecptr)[-2].o.b.i)
#define vOrig(vecptr)   ((vecptr)[-3].o.a.m)
#define vData(vecptr)   ((vecptr)[-3].o.b.p)
#define vInsv(vecptr)   ((vecptr)[-4].o.a.m)
#define vDelv(vecptr)   ((vecptr)[-4].o.b.m)

typedef struct vq_Dispatch_s {
    const char *name;                   /* type name, introspection */
    char        prefix;                 /* # of cells before vector */
    char        unit;                   /* size of single entries (<0 = bits) */
    short       flags;                  /* TODO: unused */
    void      (*cleaner)(Vector);       /* destructor function */
    vq_Type   (*getter)(int,vq_Item*);  /* getter function */
    void      (*setter)(vq_Table,int,int,const vq_Item*); /* setter function */
} Dispatch;
    
/* memory management */

Vector      (AllocVector) (Dispatch *vtab, int bytes);
void         (FreeVector) (Vector v);

Object_p      (ObjIncRef) (Object_p obj);
void          (ObjDecRef) (Object_p obj);

/* non-static internal functions */

Object_p      (DebugCode) (Object_p cmd, int objc, Object_p objv[]);
vq_Table (EmptyMetaTable) (void);
vq_Type         (GetItem) (int row, vq_Item *item);
vq_Table (ObjAsMetaTable) (Object_p obj);
vq_Table     (ObjAsTable) (Object_p obj);
int           (ObjToItem) (vq_Type type, vq_Item *item);

/* operation dispatch table */

typedef struct {
    const char *name, *args;
    vq_Type (*proc)(vq_Item*);
} CmdDispatch;

extern CmdDispatch f_commands[];

#endif
