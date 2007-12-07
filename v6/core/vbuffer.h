/*  Growable buffers.
    $Id$
    This file is part of Vlerq, see core/vlerq.h for full copyright notice.  */

#ifndef VQ_BUFFER_H
#define VQ_BUFFER_H 1

#include "v_defs.h"

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

#endif
