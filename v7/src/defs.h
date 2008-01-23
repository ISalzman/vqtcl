/*  Vlerq base definitions.
    $Id$
    This file is part of Vlerq, see src/vlerq.h for full copyright notice. */

#define VQ_UNUSED(x)    ((void)(x))	    /* to avoid warnings */
#define VQ_NULLABLE     (1 << 4)
#define VQ_TYPEMASK     (VQ_NULLABLE - 1)

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

/* treat a vector pointer v as an array with elements of type t */
#define vCast(v,t)  ((t*)(v))

/* header fields common to all views and vectors */
#define vSize(v)    vCast(v,vqInfo)[-1].size
#define vRefs(v)    vCast(v,vqInfo)[-1].refs
#define vExtra(v)   vCast(v,vqInfo)[-1].extra
#define vDisp(v)    vCast(v,vqInfo)[-1].disp

/* view-specific header fields */
#define vwState(v)  v[-1].state
#define vwAuxP(v)   v[-1].auxp
#define vwAuxI(v)   ((intptr_t) vwAuxP(v))
#define vwMeta(v)   v[-1].meta
#define vwRows(v)   vSize(v)
#define vwCols(v)   vSize(vwMeta(v))
#define vwMore(v)   ((void*) (((vqCell*) v) + vwCols(v)))

/* a view is an array of column references, stored as cells */
#define vwCol(v,i)  vCast(v,vqCell)[i]

typedef struct vqDispatch_s {
    const char *name;
    uint8_t prefix;
    char unit;
    void (*cleaner)(void*);
    vqType (*getter)(int,vqCell*);
    void (*setter)(void*,int,int,const vqCell*);
    void (*replacer)(vqView,int,int,vqView);
} vqDispatch;

/* Note: vqInfo must be defined as last field in all view and vector headers
   (therefore all views and vectors have a size and dispatch pointer) */

typedef struct vqInfo_s {
    int size;
    int refs;
    int extra;
    const vqDispatch *disp;
} vqInfo, *vqVec;
    
struct vqView_s {
    lua_State *state;
    void *auxp;
    vqView meta;
    vqInfo info;
};

/* sparse.c */

void* (VecInsert) (vqVec *vecp, int off, int cnt);
void* (VecDelete) (vqVec *vecp, int off, int cnt);

void* (RangeFlip) (vqVec *vecp, int off, int count);
int (RangeLocate) (vqVec v, int off, int *offp);
void (RangeInsert) (vqVec *vecp, int off, int count, int mode);
void (RangeDelete) (vqVec *vecp, int off, int count);

/* mutable.c */

int (IsMutable) (vqView t);
vqView (MutWrapVop) (vqView t);

/* file.c */

typedef struct vqMap_s *vqMap;

vqView (MapToView) (vqMap map);

/* buffer.c */

typedef struct Buffer Buffer, *Buffer_p;
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
vqVec (BufferAsIntVec) (Buffer *bp);
int (NextBuffer) (Buffer *bp, char **firstp, int *countp);

/* emit.c */

typedef void *(*SaveInitFun)(void*,intptr_t);
typedef void *(*SaveDataFun)(void*,const void*,intptr_t);

intptr_t (ViewSave) (vqView t, void *aux, SaveInitFun fi, SaveDataFun fd);

/* sort.c */

int (ViewCompare) (vqView view1, vqView view2);
int (RowCompare) (vqView v1, int r1, vqView v2, int r2);
int (RowEqual) (vqView v1, int r1, vqView v2, int r2);
