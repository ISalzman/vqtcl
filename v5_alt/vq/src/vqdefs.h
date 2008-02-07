/* Vlerq public C header */

#ifndef VQ_DEFS_H
#define VQ_DEFS_H

#include "vq4c.h"

#include <assert.h>

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
    
/* growable buffers */

typedef struct vq_Buffer_s *Buffer_p;
typedef struct Overflow *Overflow_p;

struct vq_Buffer_s {
    union { char *c; int *i; const void **p; } fill;
    char       *limit;
    Overflow_p  head;
    intptr_t    saved;
    intptr_t    used;
    char       *ofill;
    char       *result;
    char        buf[64];
    char        slack[8]; /* TODO: get rid of this */
};

#define ADD_ONEC_TO_BUF(b,x) (*(b).fill.c++ = (x))
#define ADD_CHAR_TO_BUF(b,x) \
          { char _c = (x); \
            if ((b).fill.c < (b).limit) *(b).fill.c++ = _c; \
              else AddToBuffer(&(b), &_c, sizeof _c); }
#define ADD_INT_TO_BUF(b,x) \
          { int _i = (x); \
            if ((b).fill.c < (b).limit) *(b).fill.i++ = _i; \
              else AddToBuffer(&(b), &_i, sizeof _i); }
#define ADD_PTR_TO_BUF(b,x) \
          { const void *_p = (x); \
            if ((b).fill.c < (b).limit) *(b).fill.p++ = _p; \
              else AddToBuffer(&(b), &_p, sizeof _p); }
#define BufferFill(b) ((b)->saved + ((b)->fill.c - (b)->buf))

void        (AddToBuffer) (Buffer_p bp, const void *data, intptr_t len);
void       *(BufferAsPtr) (Buffer_p bp, int fast);
void         (InitBuffer) (Buffer_p bp);
int          (NextBuffer) (Buffer_p bp, char **firstp, int *countp);
void      (ReleaseBuffer) (Buffer_p bp, int keep);

/* memory management */

Vector      (AllocVector) (Dispatch *vtab, int bytes);
void         (FreeVector) (Vector v);

Object_p      (ObjIncRef) (Object_p obj);
void          (ObjDecRef) (Object_p obj);

/* non-static internal functions */

Object_p      (DebugCode) (Object_p cmd, int objc, Object_p objv[]);
vq_Table (EmptyMetaTable) (void);
vq_Type         (GetItem) (int row, vq_Item *item);
vq_Table      (IotaTable) (int rows, const char *name);
int           (IsMutable) (vq_Table t);
void      (MutVecReplace) (vq_Table t, int off, int cnt, vq_Table data);
void          (MutVecSet) (Vector v, int row, int col, const vq_Item *item);
vq_Table (ObjAsMetaTable) (Object_p obj);
vq_Table     (ObjAsTable) (Object_p obj);
int           (ObjToItem) (vq_Type type, vq_Item *item);
void          (UpdateVar) (vq_Item info);
vq_Table    (WrapMutable) (vq_Table t);

/* operation dispatch table */

typedef struct {
    const char *name, *args;
    vq_Type (*proc)(vq_Item*);
} CmdDispatch;

extern CmdDispatch f_commands[];

#endif
