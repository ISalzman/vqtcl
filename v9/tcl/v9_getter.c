/* V9 fixed vector getters
   2009-05-08 <jcw@equU4.com> http://opensource.org/licenses/mit-license.php
   $Id$ */
 
#include "v9.priv.h"
#include <string.h>

static V9Types getterU0 (int row, V9Item* pitem) {
    pitem->i = 0;
    return V9T_int;
}

static V9Types getterU1 (int row, V9Item* pitem) {
    const char *ptr = Address(pitem->c.p);
    pitem->i = (ptr[row>>3] >> (row&7)) & 1;
    return V9T_int;
}

static V9Types getterU2 (int row, V9Item* pitem) {
    const char *ptr = Address(pitem->c.p);
    pitem->i = (ptr[row>>2] >> 2*(row&3)) & 3;
    return V9T_int;
}

static V9Types getterU4 (int row, V9Item* pitem) {
    const char *ptr = Address(pitem->c.p);
    pitem->i = (ptr[row>>1] >> 4*(row&1)) & 15;
    return V9T_int;
}

static V9Types getterI8 (int row, V9Item* pitem) {
    const char *ptr = Address(pitem->c.p);
    pitem->i = (int8_t) ptr[row];
    return V9T_int;
}

#if VALUES_MUST_BE_ALIGNED+0

static V9Types getterI16 (int row, V9Item* pitem) {
    const uint8_t *ptr = (const uint8_t*) Address(pitem->c.p) + row * 2;
#if _BIG_ENDIAN+0
    pitem->i = (((int8_t) ptr[0]) << 8) | ptr[1];
#else
    pitem->i = (((int8_t) ptr[1]) << 8) | ptr[0];
#endif
    return V9T_int;
}

static V9Types getterI32 (int row, V9Item* pitem) {
    const char *ptr = Address(pitem->c.p) + row * 4;
    int i;
    for (i = 0; i < 4; ++i)
        pitem->r[i] = ptr[i];
    return V9T_int;
}

static V9Types getterI64 (int row, V9Item* pitem) {
    const char *ptr = Address(pitem->c.p) + row * 8;
    int i;
    for (i = 0; i < 8; ++i)
        pitem->r[i] = ptr[i];
    return V9T_long;
}

static V9Types getterF32 (int row, V9Item* pitem) {
    getterI32(row, pitem);
    return V9T_float;
}

static V9Types getterF64 (int row, V9Item* pitem) {
    getterI64(row, pitem);
    return V9T_double;
}

#else

static V9Types getterI16 (int row, V9Item* pitem) {
    const char *ptr = Address(pitem->c.p);
    pitem->i = ((short*) ptr)[row];
    return V9T_int;
}

static V9Types getterI32 (int row, V9Item* pitem) {
    const char *ptr = Address(pitem->c.p);
    pitem->i = ((const int*) ptr)[row];
    return V9T_int;
}

static V9Types getterI64 (int row, V9Item* pitem) {
    const char *ptr = Address(pitem->c.p);
    pitem->l = ((const int64_t*) ptr)[row];
    return V9T_long;
}

static V9Types getterF32 (int row, V9Item* pitem) {
    const char *ptr = Address(pitem->c.p);
    pitem->f = ((const float*) ptr)[row];
    return V9T_float;
}

static V9Types getterF64 (int row, V9Item* pitem) {
    const char *ptr = Address(pitem->c.p);
    pitem->d = ((const double*) ptr)[row];
    return V9T_double;
}

#endif

static V9Types getterI16R (int row, V9Item* pitem) {
    const uint8_t *ptr = (const uint8_t*) Address(pitem->c.p) + row * 2;
#if _BIG_ENDIAN+0
    pitem->i = (((int8_t) ptr[1]) << 8) | ptr[0];
#else
    pitem->i = (((int8_t) ptr[0]) << 8) | ptr[1];
#endif
    return V9T_int;
}

static V9Types getterI32R (int row, V9Item* pitem) {
    const char *ptr = Address(pitem->c.p) + row * 4;
    int i;
    for (i = 0; i < 4; ++i)
        pitem->r[i] = ptr[3-i];
    return V9T_int;
}

static V9Types getterI64R (int row, V9Item* pitem) {
    const char *ptr = Address(pitem->c.p) + row * 8;
    int i;
    for (i = 0; i < 8; ++i)
        pitem->r[i] = ptr[7-i];
    return V9T_long;
}

static V9Types getterF32R (int row, V9Item* pitem) {
    getterI32R(row, pitem);
    return V9T_float;
}

static V9Types getterF64R (int row, V9Item* pitem) {
    getterI64R(row, pitem);
    return V9T_double;
}

static struct VectorType vtU0   = { "U0",   0,  0, getterU0   };
static struct VectorType vtU1   = { "U1",   -1, 0, getterU1   };
static struct VectorType vtU2   = { "U2",   -2, 0, getterU2   };
static struct VectorType vtU4   = { "U4",   -3, 0, getterU4   };
static struct VectorType vtI8   = { "I8",   1,  0, getterI8   };
static struct VectorType vtI16  = { "I16",  2,  0, getterI16  };
static struct VectorType vtI32  = { "I32",  4,  0, getterI32  };
static struct VectorType vtI64  = { "I64",  8,  0, getterI64  };
static struct VectorType vtI16R = { "I16R", 2,  0, getterI16R };
static struct VectorType vtI32R = { "I32R", 4,  0, getterI32R };
static struct VectorType vtI64R = { "I64R", 8,  0, getterI64R };
static struct VectorType vtF32  = { "F32",  4,  0, getterF32  };
static struct VectorType vtF64  = { "F64",  8,  0, getterF64  };
static struct VectorType vtF32R = { "F32R", 4,  0, getterF32R };
static struct VectorType vtF64R = { "F64R", 8,  0, getterF64R };

VectorType* PickIntGetter (int bits) {
    switch (bits) {
        default:    /* fall through */
        case 0:     return &vtU0;
        case 1:     return &vtU1;
        case 2:     return &vtU2;
        case 4:     return &vtU4;
        case 8:     return &vtI8;
        case 16:    return &vtI16;
        case 32:    return &vtI32;
        case 64:    return &vtI64;
    }
}

VectorType* FixedGetter (int bytes, int rows, int real, int flip) {
    int bits;

    static char widths[8][7] = {
        {0,-1,-1,-1,-1,-1,-1},
        {0, 8,16, 1,32, 2, 4},
        {0, 4, 8, 1,16, 2,-1},
        {0, 2, 4, 8, 1,-1,16},
        {0, 2, 4,-1, 8, 1,-1},
        {0, 1, 2, 4,-1, 8,-1},
        {0, 1, 2, 4,-1,-1, 8},
        {0, 1, 2,-1, 4,-1,-1},
    };

    bits = rows < 8 && bytes < 7 ? widths[rows][bytes] : (bytes << 3) / rows;

    switch (bits) {
        case 16:    return flip ? &vtI16R : &vtI16;
        case 32:    return real ? flip ? &vtF32R : &vtF32
                                : flip ? &vtI32R : &vtI32;
        case 64:    return real ? flip ? &vtF64R : &vtF64
                                : flip ? &vtI64R : &vtI64;
    }

    return PickIntGetter(bits);
}

V9Types GetItem (int row, V9Item* pitem) {
    Vector v = pitem->c.p;
    if (v == 0)
        return V9T_nil;
    if (row < 0)
        row += Head(v).count;
    if (row < 0 || row >= Head(v).count)
        return V9T_nil;
    assert(Head(v).type->getter != 0);
    return Head(v).type->getter(row, pitem);
}

V9Item V9_Get (V9View v, int row, int col, V9Item* pdef) {
    V9Item pitem;
    int ncols = Head(v->meta).count;
    if (col < 0)
        col += ncols;
    if (0 <= row && row < Head(v).count && 0 <= col && col < ncols) {
	    pitem = v->col[col];
		V9Types t = GetItem(row, &pitem);
		if (t != V9T_nil)
			return pitem;
	}
	if (pdef != 0)
		pitem = *pdef;
	else
		memset(&pitem, 0, sizeof pitem);
	return pitem;
}
