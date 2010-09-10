/* V9 private definitions
   2009-05-08 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
   $Id$ */
 
#include "v9.h"
#include <assert.h>

#define UNUSED(x)    ((void)(x))	    /* to avoid warnings */

typedef struct VectorHead* Vector;

typedef const struct VectorAlloc {
    const char*   name;
    void        (*release)(Vector);
    const char* (*address)(Vector);
} VectorAlloc;

typedef const struct VectorType {
    const char*   name;
    int           itembytes;
    void        (*cleaner)(Vector);
    V9Types     (*getter)(int,V9Item*);
    void        (*setter)(Vector,int,int,const V9Item*);
    void        (*replacer)(V9View,int,int,V9View);
    Vector      (*duplicator)(Vector);
} VectorType;

struct VectorHead {
    VectorAlloc* alloc;
    VectorType*  type;
    long         count;
    long         shares;
};

struct ViewVector {
    V9View meta;
    void* extra;
    V9Item col[]; // must be last member
};

#define Head(p)         (((Vector)(p))[-1])
#define IsShared(p)     (Head(p).shares > 0)
#define Address(p)      (Head(p).alloc->address)(p)

// dynamic memory buffers

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

#define BufferCount(b) ((b)->saved + ((b)->fill.c - (b)->buf))

// v9_alloc.c
Vector IncRef (Vector vp);
Vector DecRef (Vector vp);
void Unshare (Vector* vpp);
Vector NewInlineVector (long length);
// Vector NewFixedVector (const char* ptr, long length);
// Vector NewDynamicVector ();
Vector NewMappedVector (int fd);
Vector NewSharedVector (Vector vp, long offset, long length);

// v9_buffer.c
void InitBuffer (Buffer* bp);
void ReleaseBuffer (Buffer* bp, int keep);
void AddToBuffer (Buffer* bp, const void* data, intptr_t len);
int NextFromBuffer (Buffer* bp, char** firstp, int* pcount);
void* BufferAsPtr (Buffer* bp, int fast);

// v9_getter.c
VectorType* PickIntGetter (int bits);
VectorType* FixedGetter (int bytes, int rows, int real, int flip);
V9Types GetItem (int row, V9Item* pitem);

// v9_setter.c
Vector NewDataVec (V9Types type, int rows);
void DataViewReplacer (V9View v, int off, int del, V9View vnew);

// v9_view.c
V9View EmptyMeta ();
V9View IndirectView (V9View meta, int nrows, VectorType* type, int bytes);
