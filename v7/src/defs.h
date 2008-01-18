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

/* access to header fields, i.e. in memory preceding the vector contents */
#define vHead(v,f)  (v)[-1].f

/* header fields common to all views and vectors */
#define vSize(v)    vHead(vCast(v,vqInfo),size)
#define vRefs(v)    vHead(vCast(v,vqInfo),refs)
#define vExtra(v)   vHead(vCast(v,vqInfo),extra)
#define vDisp(v)    vHead(vCast(v,vqInfo),disp)

/* view-specific header fields */
#define vwState(v)  vHead(v,state)
#define vwAuxP(v)   vHead(v,auxp)
#define vwAuxI(v)   ((intptr_t) vwAuxP(v))
#define vwMeta(v)   vHead(v,meta)
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

void* (VecInsert) (vqVec *vecp, int off, int cnt);
void* (VecDelete) (vqVec *vecp, int off, int cnt);

void* (RangeFlip) (vqVec *vecp, int off, int count);
int (RangeLocate) (vqVec v, int off, int *offp);
void (RangeInsert) (vqVec *vecp, int off, int count, int mode);
void (RangeDelete) (vqVec *vecp, int off, int count);

int (IsMutable) (vqView t);
vqView (MutWrapVop) (vqView t);