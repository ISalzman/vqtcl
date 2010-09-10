/* V9 mutable vector setters
   2009-05-08 <jcw@equU4.com> http://opensource.org/licenses/mit-license.php
   $Id$ */
 
#include "v9.priv.h"

#include <stdlib.h>
#include <string.h>

static V9Types NilVecGetter (int row, V9Item *cp) {
    const char* p = cp->p;
    cp->i = (p[row/8] >> (row&7)) & 1;
    return cp->i ? V9T_int : V9T_nil;
}

static V9Types IntVecGetter (int row, V9Item *cp) {
    const int* p = cp->p;
    cp->i = p[row];
    return V9T_int;
}

static V9Types LongVecGetter (int row, V9Item *cp) {
    const int64_t* p = cp->p;
    cp->l = p[row];
    return V9T_long;
}

static V9Types FloatVecGetter (int row, V9Item *cp) {
    const float* p = cp->p;
    cp->f = p[row];
    return V9T_float;
}

static V9Types DoubleVecGetter (int row, V9Item *cp) {
    const double* p = cp->p;
    cp->d = p[row];
    return V9T_double;
}

static V9Types StringVecGetter (int row, V9Item *cp) {
    const char** p = cp->p;
    cp->s = p[row] != 0 ? p[row] : "";
    return V9T_string;
}

static V9Types BytesVecGetter (int row, V9Item *cp) {
    V9Item* p = cp->p;
    *cp = p[row];
    return V9T_bytes;
}

static V9Types ViewVecGetter (int row, V9Item *cp) {
    V9View* p = cp->p;
    cp->p = p[row];
    return V9T_view;
}

static void NilVecSetter (Vector q, int row, int col, const V9Item *cp) {
    int bit = 1 << (row&7);
    UNUSED(col);
    if (cp != 0)
        ((char*) q)[row/8] |= bit;
    else
        ((char*) q)[row/8] &= ~bit;
}

static void IntVecSetter (Vector q, int row, int col, const V9Item *cp) {
    UNUSED(col);
    ((int*) q)[row] = cp != 0 ? cp->i : 0;
}

static void LongVecSetter (Vector q, int row, int col, const V9Item *cp) {
    UNUSED(col);
    ((int64_t*) q)[row] = cp != 0 ? cp->l : 0;
}

static void FloatVecSetter (Vector q, int row, int col, const V9Item *cp) {
    UNUSED(col);
    ((float*) q)[row] = cp != 0 ? cp->f : 0;
}

static void DoubleVecSetter (Vector q, int row, int col, const V9Item *cp) {
    UNUSED(col);
    ((double*) q)[row] = cp != 0 ? cp->d : 0;
}

static void StringVecSetter (Vector q, int row, int col, const V9Item *cp) {
    const char** p = (const char**) q;
    UNUSED(col);
    free((void*) p[row]);
    p[row] = cp != 0 ? strcpy(malloc(strlen(cp->s)+1), cp->s) : 0;
}

static void BytesVecSetter (Vector q, int row, int col, const V9Item *cp) {
    V9Item* p = (V9Item*) q;
    UNUSED(col);
    free(p[row].p);
    if (cp != 0) {
        p[row].p = memcpy(malloc(cp->c.i), cp->p, cp->c.i);
        p[row].c.i = cp->c.i;
    } else {
        p[row].p = 0;
        p[row].c.i = 0;
    }
}

static void ViewVecSetter (Vector q, int row, int col, const V9Item *cp) {
    V9View orig = ((V9View*) q)[row];
    UNUSED(col);
    //XXX view_detach(q, row);
    ((V9View*) q)[row] = cp != 0 ? V9_AddRef(cp->p) : 0;
    //XXX view_attach(q, row);
    V9_Release(orig);
}

static void StringVecCleaner (Vector cp) {
    const char** p = (const char**) cp;
    int i;
    for (i = 0; i < Head(cp).count; ++i)
        free((void*) p[i]);
}

static void BytesVecCleaner (Vector cp) {
    V9Item* p = (V9Item*) cp;
    int i;
    for (i = 0; i < Head(cp).count; ++i)
        free(p[i].p);
}

static void ViewVecCleaner (Vector cp) {
    Vector* p = (Vector*) cp;
    int i;
    for (i = 0; i < Head(cp).count; ++i)
        DecRef(p[i]);
}

static Vector VecClone (Vector vp, int copybytes, int allocbytes) {
    Vector vnew = NewInlineVector(allocbytes);
    memcpy(vnew - 1, vp - 1, copybytes + sizeof *vp);
    Head(vnew).shares = -1;
    assert(Head(vp).type->itembytes > 0);
    Head(vnew).count = allocbytes / Head(vp).type->itembytes;
    return vnew;
}

static Vector DataVecDuplicator (Vector vp) {
    int bytes = Head(vp).count * Head(vp).type->itembytes;
    return VecClone(vp, bytes, bytes);
}

static Vector ViewVecDuplicator (Vector vp) {
    int bytes = Head(vp).count * Head(vp).type->itembytes;
    Vector vnew = VecClone(vp, bytes, bytes);
    int r, nrows = Head(vnew).count;
    for (r = 0; r < nrows; ++r)
        V9_AddRef(((V9View*) vnew)[r]);
    return vnew;
}

static VectorType nvtab = {
    "nilvec", -1, 0, NilVecGetter, NilVecSetter, 0, DataVecDuplicator
};

static VectorType ivtab = {
    "intvec", 4, 0, IntVecGetter, IntVecSetter, 0, DataVecDuplicator
};

static VectorType lvtab = {
    "longvec", 8, 0, LongVecGetter, LongVecSetter, 0, DataVecDuplicator
};

static VectorType fvtab = {
    "floatvec", 4, 0, FloatVecGetter, FloatVecSetter, 0, DataVecDuplicator 
};

static VectorType dvtab = {
    "doublevec", 8, 0, DoubleVecGetter, DoubleVecSetter, 0, DataVecDuplicator
};

static VectorType svtab = {
    "stringvec", sizeof(void*), 
    StringVecCleaner, StringVecGetter, StringVecSetter, 0, DataVecDuplicator
};

static VectorType bvtab = {
    "bytesvec", sizeof(V9Item), 
    BytesVecCleaner, BytesVecGetter, BytesVecSetter, 0, DataVecDuplicator
};

static VectorType tvtab = {
    "viewvec", sizeof(void*), 
    ViewVecCleaner, ViewVecGetter, ViewVecSetter, 0, ViewVecDuplicator
};

Vector NewDataVec (V9Types type, int rows) {
    int bytes;
    VectorType* vtab;
    
    switch (type) {
        case V9T_nil:
            vtab = &nvtab; bytes = ((rows+31)/32) * sizeof(int); break;
        case V9T_int:
            vtab = &ivtab; bytes = rows * sizeof(int); break;
        case V9T_long:
            vtab = &lvtab; bytes = rows * sizeof(int64_t); break;
        case V9T_float:
            vtab = &fvtab; bytes = rows * sizeof(float); break;
        case V9T_double:
            vtab = &dvtab; bytes = rows * sizeof(double); break;
        case V9T_string:
            vtab = &svtab; bytes = rows * sizeof(const char*); break;
        case V9T_bytes:
            vtab = &bvtab; bytes = rows * sizeof(V9Item); break;
        case V9T_view:
            vtab = &tvtab; bytes = rows * sizeof(V9View*); break;
    }
    
    Vector vp = NewInlineVector(bytes);
    Head(vp).type = vtab;
    Head(vp).count = /* vLimit(v) = */ rows;
    return vp;
}

V9View V9_Set (V9View v, int row, int col, V9Item* pitem) {
    int nrows = Head(v).count, ncols = Head(v->meta).count;
    if (row < 0)
        row += nrows;
    if (col < 0)
        col += ncols;
    Unshare((Vector*) &v);
    assert(Head(v).type->setter != 0);
    Head(v).type->setter((Vector) v, row, col, pitem);
    return v;
}

static void VecInsert (V9Item* pcol, int off, int cnt, int end) {
    Vector vp = pcol->c.p;
    int unit = Head(vp).type->itembytes;
    if (unit > 0) {
        int alloc = Head(vp).count * unit;
        off *= unit;
        cnt *= unit;
        end *= unit;
        char* ptr = (char*) Address(vp);
        if (end + cnt <= alloc) {
            memmove(ptr + off + cnt, ptr + off, end - off);
            memset(ptr + off, 0, cnt);
        } else {
            Vector vnew = VecClone(vp, off, end + cnt); //TODO slack
            char* newptr = (char*) Address(vnew);
            memset(newptr + off, 0, cnt);
            memcpy(newptr + off + cnt, ptr + off, end - off);
            DecRef(vp);
            pcol->c.p = IncRef(vnew);
        }
    }
    //TODO else handle bit vector insertion
}

static void VecDelete (V9Item* pcol, int off, int cnt, int end) {
    Vector vp = pcol->c.p;
    int unit = Head(vp).type->itembytes;
    if (unit > 0) {
        //TODO resize if new contents needs much less than alloc size
        off *= unit;
        cnt *= unit;
        end *= unit;
        char* ptr = (char*) Address(vp);
        memmove(ptr + off, ptr + off + cnt, end - off - cnt);
        memset(ptr + end - cnt, 0, cnt);
    }
    //TODO else handle bit vector deletion
}

void DataViewReplacer (V9View v, int off, int del, V9View vnew) {
    int ncols = Head(v->meta).count, nrows = Head(v).count;
    int r, c, newcols = 0, ins = 0;
    if (vnew != 0) {
        newcols = Head(vnew->meta).count;
        ins = Head(vnew).count;
    }
    for (c = 0; c < ncols; ++c) {
        if (ins > del)
            VecInsert(&v->col[c], off + del, ins - del, nrows);
        else if (del > ins)
            VecDelete(&v->col[c], off, del - ins, nrows);
        //TODO could optimize for vec-wise getter/setter, perhaps also batched
        for (r = 0; r < ins; ++r)
            if (c < newcols) {
                V9Item item = V9_Get(vnew, r, c, 0);
                V9_Set(v, off + r, c, &item);
            } else
                V9_Set(v, off + r, c, 0);
    }
    Head(v).count += ins - del;
}

V9View V9_Replace (V9View v, int offset, int count, V9View vnew) {
    int nrows = Head(v).count; //, ncols = Head(v->meta).count;
    if (offset < 0)
        offset += nrows;
    Unshare((Vector*) &v);
    Head(v).type->replacer(v, offset, count, vnew);
    return v;
}
