/*  Vlerq base definitions.
    $Id$
    This file is part of Vlerq, see src/vlerq.h for full copyright notice. */

#ifndef VQ_FULL
#define VQ_FULL 1 /* default includes all code, not just the minimal core */
#endif

#define VQ_UNUSED(x)    ((void)(x))	    /* to avoid warnings */
#define VQ_NULLABLE     (1 << 4)
#define VQ_TYPEMASK     (VQ_NULLABLE - 1)

/* portability */

#if defined(__WIN32)
#define VQ_WIN32
#endif

#if defined(__sparc__) || defined(__sgi__) || defined(__arm__)
#define VQ_MUSTALIGN
#endif

#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
#define VQ_BIGENDIAN
#endif

typedef lua_State *vqEnv;
    
/* treat a vector pointer v as an array with elements of type t */
#define vCast(v,t)  ((t*)(v))

typedef struct vqDispatch_s {
    const char *typename;
    uint8_t     headerbytes;
    char        itembytes; /* 0 if not applicable, < 0 if not byte-sized */
    void      (*cleaner) (vqColumn);
    vqType    (*getter) (int,vqSlot*);
    void      (*setter) (vqColumn,int,int,const vqSlot*);
    void      (*replacer) (vqView,int,int,vqView); /* TODO: view vs. col? */
} vqDispatch;

/* Note: vqInfo must be the *last* field in all view and vector headers */

/* header fields common to all views and vectors */
#define vInfo(v)    (v)->info[-1]
#define vSize(v)    (v)->info[-1].currsize

struct vqInfo_s {
    const vqDispatch *dispatch;
    union { vqColumn col; vqView view; } owner;
    int ownerpos;
    int currsize;
};

/* view-specific access */
#define vwEnv(v)    (v)->vhead[-1].env
#define vwRefs(v)   (v)->vhead[-1].refs
#define vwAuxP(v)   (v)->vhead[-1].auxp
#define vwMeta(v)   (v)->vhead[-1].meta
#define vwCols(v)   vSize(vwMeta(v))
#define vwMore(v)   ((void*) ((v)->col + vwCols(v)))

struct vqView_s {
    vqEnv            env;
    int              refs;
    void            *auxp;
    vqView           meta;
    struct vqInfo_s  _info;
};

union vqView_u {
    vqSlot          col[1]; /* payload, columns indexed as 0 .. vwCols(v)-1 */
    struct vqInfo_s info[1]; /* common header, accessed as info[-1] */
    struct vqView_s vhead[1]; /* view-specific header, accessed as vhead[-1] */
};

union vqColumn_u {
        /* various ways to access the column payload: */
    char            chars[1];
    int             ints[1];
    int64_t         longs[1];
    float           floats[1];
    double          doubles[1];
    const char     *strings[1];
    vqSlot          slots[1];
    vqView          views[1];
        /* other column accesses: */
    union vqView_u  asview[1]; /* never indexed, used to cast column to view */
    struct vqInfo_s info[1]; /* common header, accessed as info[-1] */
};

/* TODO: limit can probably be dropped, use col's vs. view's currsize instead */
#define vLimit(v)   vCast(v,vqData)[-1].limit

typedef struct vqData_s {
    int              limit;
    struct vqInfo_s  _info;
} vqData;

/* nulls.c */

void* (VecInsert) (vqColumn *vecp, int off, int cnt);
void* (VecDelete) (vqColumn *vecp, int off, int cnt);

void* (RangeFlip) (vqColumn *vecp, int off, int count);
int (RangeLocate) (vqColumn v, int off, int *offp);
void (RangeInsert) (vqColumn *vecp, int off, int count, int mode);
void (RangeDelete) (vqColumn *vecp, int off, int count);

/* mutable.c */

int (IsMutable) (vqView t);
vqView (MutWrapVop) (vqView t);

/* file.c */

vqView (LoadView) (vqView map);

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
vqView (BufferAsPosView) (vqEnv env, Buffer_p bp);
int (NextBuffer) (Buffer *bp, char **firstp, int *countp);

/* emit.c */

typedef void *(*SaveInitFun)(void*,intptr_t);
typedef void *(*SaveDataFun)(void*,const void*,intptr_t);

intptr_t (ViewSave) (vqView t, void *aux, SaveInitFun fi, SaveDataFun fd);

/* sort.c */

int (ViewCompare) (vqView view1, vqView view2);
int (RowCompare) (vqView v1, int r1, vqView v2, int r2);
int (RowEqual) (vqView v1, int r1, vqView v2, int r2);
