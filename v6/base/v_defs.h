/*  Vlerq private C header.
    $Id$
    This file is part of Vlerq, see base/vlerq.h for full copyright notice.  */

#ifndef VQ_INTERN_H
#define VQ_INTERN_H

#include "vlerq.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* portability */

#if defined(__WIN32)
#define VQ_WIN32
#endif

#if defined(__sparc__) || defined(__sgi__)
#define VQ_MUSTALIGN
#endif

#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
#define VQ_BIGENDIAN
#endif

#if !defined(_TCL)
#define ObjToItem(t,i) 0
#endif

#define VQ_UNUSED(x)    ((void)(x))	/* to avoid warnings */

/* definitions for use with vq_Type */

#define VQ_NULLABLE (1 << 4)
#define VQ_TYPEMASK (VQ_NULLABLE - 1)

/* every vq_View is a Vector, but not vice-versa */

typedef vq_View Vector;

/* view prefix fields and type dispatch */

#define vType(vecptr)   ((vecptr)[-1].o.a.h)
#define vRefs(vecptr)   ((vecptr)[-1].o.b.i)

#define vMeta(vecptr)   ((vecptr)[-2].o.a.v)    /* same slot as vLimit */
#define vLimit(vecptr)  ((vecptr)[-2].o.a.i)    /* same slot as vMeta */
#define vCount(vecptr)  ((vecptr)[-2].o.b.i)

#define vOrig(vecptr)   ((vecptr)[-3].o.a.v)    /* same slot as vOffs */
#define vOffs(vecptr)   ((vecptr)[-3].o.a.i)    /* same slot as vOrig */
#define vData(vecptr)   ((vecptr)[-3].o.b.v)

#define vInsv(vecptr)   ((vecptr)[-4].o.a.v)
#define vDelv(vecptr)   ((vecptr)[-4].o.b.v)

#define vOref(vecptr)   ((vecptr)[-5].o.a.p)
#define vPerm(vecptr)   ((vecptr)[-5].o.b.p)

typedef struct vq_Dispatch_s {
    const char *name;                   /* type name, introspection */
    char        prefix;                 /* # of cells before vector */
    char        unit;                   /* size of single entries (<0 = bits) */
    short       flags;                  /* TODO: unused */
    void      (*cleaner)(Vector);       /* destructor function */
    vq_Type   (*getter)(int,vq_Item*);  /* getter function */
    void      (*setter)(vq_View,int,int,const vq_Item*);
    void      (*replacer)(vq_View,int,int,vq_View);
} Dispatch;
    
/* host language functions */

#if defined (_TCL)
typedef struct Tcl_Obj *Object_p;
#else
typedef void           *Object_p;
#endif

Object_p (ObjRetain) (Object_p obj);
void (ObjRelease) (Object_p obj);

void (UpdateVar) (Object_p ref, Object_p val);
vq_View (ObjAsMetaView) (void *ip, Object_p obj);
int (ObjToItem) (vq_Type type, vq_Item *item);
Object_p (ChangesAsList) (vq_View view);
Object_p (MutableObject) (Object_p obj);
Object_p (ItemAsObj) (vq_Type type, vq_Item item);

vq_Type (DataCmd_VIX) (vq_Item a[]);

/* memory management in core.c */

Vector (AllocVector) (Dispatch *vtab, int bytes);
void (FreeVector) (Vector v);

/* core view functions in core.c */

vq_Type (GetItem) (int row, vq_Item *item);

/* data vectors in core.c */

Vector (AllocDataVec) (vq_Type type, int rows);

/* view creation in core.c */

vq_View (EmptyMetaView) (void);
void (IndirectCleaner) (Vector v);
vq_View (IndirectView) (vq_View meta, Dispatch *vtab, int rows, int extra);

/* utility wrappers in core.c */

int (CharAsType) (char c);
int (StringAsType) (const char *str);
const char* (TypeAsString) (int type, char *buf);

/* operator dispatch in core.c */

typedef struct {
    const char *name, *args;
    vq_Type (*proc)(vq_Item*);
} CmdDispatch;

extern CmdDispatch f_commands[];

/* vvopdef.c */

vq_View (IotaView) (int rows, const char *name, int base);
vq_View (PassView) (vq_View v);

#endif
