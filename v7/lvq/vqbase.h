/*  Vlerq base definitions.
    $Id$
    This file is part of Vlerq, see lvq/vlerq.h for full copyright notice. */

#define VQ_UNUSED(x)    ((void)(x))	    /* to avoid warnings */

/* treat a vector pointer v as an array with elements of type t */
#define vCast(v,t)  ((t*)(v))

/* access to header fields, i.e. in memory preceding the vector contents */
#define vHead(v,f)  (v)[-1].f

/* header fields common to all views and vectors */
#define vSize(v)    vHead(vCast(v,vqInfo),size)
#define vDisp(v)    vHead(vCast(v,vqInfo),disp)

/* view-specific header fields */
#define vwState(v)  vHead(v,state)
#define vwMeta(v)   vHead(v,meta)
#define vwOrig(v)   vHead(v,orig)
#define vwRows(v)   vSize(v)
#define vwCols(v)   vSize(vwMeta(v))

/* a view is an array of column references, stored as cells */
#define vwCol(v,i)  vCast(v,vqCell)[i]

/* Note: vqInfo must be defined as last field in all view and vector headers
   (therefore all views and vectors have a size and dispatch pointer) */

typedef struct vqInfo_s vqInfo, *vqVec;
    
typedef struct vqDispatch_s {
    const char *name;
    uint8_t prefix;
    char unit;
    void (*cleaner)(vqVec);
    vqType (*getter)(int,vqCell*);
    void (*setter)(void*,int,int,const vqCell*);
    void (*replacer)(vqView,int,int,vqView);
} vqDispatch;

struct vqInfo_s {
    int size;
    const vqDispatch *disp;
};
    
struct vqView_s {
    lua_State *state;
    vqView meta;
    vqView orig;
    vqInfo info;
};
