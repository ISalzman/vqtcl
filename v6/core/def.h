/*  Vlerq private C header.
    $Id$
    This file is part of Vlerq, see core/vlerq.h for full copyright notice.  */

#ifndef VQ_DEFS_H
#define VQ_DEFS_H

#include "vlerq.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* modules included */

#ifndef VQ_MOD_ALL
#define VQ_MOD_ALL 1
#endif
#ifndef VQ_MOD_LOAD_H
#define VQ_MOD_LOAD_H VQ_MOD_ALL
#endif
#ifndef VQ_MOD_MUTABLE_H
#define VQ_MOD_MUTABLE_H VQ_MOD_ALL
#endif
#ifndef VQ_MOD_OPDEF_H
#define VQ_MOD_OPDEF_H VQ_MOD_ALL
#endif
#ifndef VQ_MOD_RANGES_H
#define VQ_MOD_RANGES_H VQ_MOD_ALL
#endif
#ifndef VQ_MOD_SAVE_H
#define VQ_MOD_SAVE_H VQ_MOD_ALL
#endif

/* portability */

#if defined(__sparc__) || defined(__sgi__)
#define VQ_MUSTALIGN 1
#endif

#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
#define VQ_BIGENDIAN 1
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

/* reader.c */

Vector (OpenMappedFile) (const char *filename);
Vector (OpenMappedBytes) (const void *data, int length, Object_p ref);
const char* (AdjustMappedFile) (Vector map, int offset);
Dispatch* (PickIntGetter) (int bits);
Dispatch* (FixedGetter) (int bytes, int rows, int real, int flip);

/* load.c */

vq_View (DescToMeta) (const char *desc, int length);
vq_View (MapToView) (Vector map);

vq_Type (Desc2MetaCmd_S) (vq_Item a[]);
vq_Type (OpenCmd_S) (vq_Item a[]);

vq_Type (LoadCmd_O) (vq_Item a[]);

/* nullable.c */

void* (VecInsert) (Vector *vecp, int off, int cnt);
void* (VecDelete) (Vector *vecp, int off, int cnt);

void* (RangeFlip) (Vector *vecp, int off, int count);
int (RangeLocate) (Vector v, int off, int *offp);
void (RangeInsert) (Vector *vecp, int off, int count, int mode);
void (RangeDelete) (Vector *vecp, int off, int count);

vq_Type (RflipCmd_OII) (vq_Item a[]);
vq_Type (RlocateCmd_OI) (vq_Item a[]);
vq_Type (RinsertCmd_OIII) (vq_Item a[]);
vq_Type (RdeleteCmd_OII) (vq_Item a[]);

/* mutable.c */

int (IsMutable) (vq_View t);
vq_View (WrapMutable) (vq_View t, Object_p o);

vq_Type (ReplaceCmd_OIIV) (vq_Item a[]);
vq_Type (SetCmd_OIIO) (vq_Item a[]);
vq_Type (UnsetCmd_OII) (vq_Item a[]);

/* vopdef.c */

vq_View (IotaView) (int rows, const char *name, int base);
vq_View (PassView) (vq_View v);

/* buffer.c */

typedef struct Buffer Buffer;
typedef struct Overflow *Overflow_p;

struct Buffer {
    union { char *c; int *i; const void **p; } fill;
    char       *limit;
    Overflow_p  head;
    intptr_t    saved;
    intptr_t    used;
    char       *ofill;
    char       *result;
    char        buf [128];
    char        slack [8];
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

void (InitBuffer) (Buffer *bp);
void (ReleaseBuffer) (Buffer *bp, int keep);
void (AddToBuffer) (Buffer *bp, const void *data, intptr_t len);
void* (BufferAsPtr) (Buffer *bp, int fast);
Vector (BufferAsIntVec) (Buffer *bp);
int (NextBuffer) (Buffer *bp, char **firstp, int *countp);

/* save.c */

typedef void *(*SaveInitFun)(void*,intptr_t);
typedef void *(*SaveDataFun)(void*,const void*,intptr_t);

intptr_t (ViewSave) (vq_View t, void *aux, SaveInitFun fi, SaveDataFun fd);

vq_Type (Meta2DescCmd_V) (vq_Item a[]);
vq_Type (EmitCmd_V) (vq_Item a[]);

#endif
