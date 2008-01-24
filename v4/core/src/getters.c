/*
 * getters.c - Implementation of several simple getter functions.
 */

#include <stdlib.h>

#include "intern.h"

static ItemTypes Getter_i0 (int row, Item_p item) {
    item->i = 0;
    return IT_int;
}

static ItemTypes Getter_i1 (int row, Item_p item) {
    const char *ptr = item->c.seq->data[0].p;
    item->i = (ptr[row>>3] >> (row&7)) & 1;
    return IT_int;
}

static ItemTypes Getter_i2 (int row, Item_p item) {
    const char *ptr = item->c.seq->data[0].p;
    item->i = (ptr[row>>2] >> 2*(row&3)) & 3;
    return IT_int;
}

static ItemTypes Getter_i4 (int row, Item_p item) {
    const char *ptr = item->c.seq->data[0].p;
    item->i = (ptr[row>>1] >> 4*(row&1)) & 15;
    return IT_int;
}

static ItemTypes Getter_i8 (int row, Item_p item) {
    const char *ptr = item->c.seq->data[0].p;
    item->i = (int8_t) ptr[row];
    return IT_int;
}

#if VALUES_MUST_BE_ALIGNED+0

static ItemTypes Getter_i16 (int row, Item_p item) {
    const Byte_t *ptr = (const Byte_t*) item->c.seq->data[0].p + row * 2;
#if _BIG_ENDIAN+0
    item->i = (((int8_t) ptr[0]) << 8) | ptr[1];
#else
    item->i = (((int8_t) ptr[1]) << 8) | ptr[0];
#endif
    return IT_int;
}

static ItemTypes Getter_i32 (int row, Item_p item) {
    const char *ptr = (const char*) item->c.seq->data[0].p + row * 4;
    int i;
    for (i = 0; i < 4; ++i)
        item->b[i] = ptr[i];
    return IT_int;
}

static ItemTypes Getter_i64 (int row, Item_p item) {
    const char *ptr = (const char*) item->c.seq->data[0].p + row * 8;
    int i;
    for (i = 0; i < 8; ++i)
        item->b[i] = ptr[i];
    return IT_wide;
}

static ItemTypes Getter_f32 (int row, Item_p item) {
    Getter_i32(row, item);
    return IT_float;
}

static ItemTypes Getter_f64 (int row, Item_p item) {
    Getter_i64(row, item);
    return IT_double;
}

#else

static ItemTypes Getter_i16 (int row, Item_p item) {
    const char *ptr = item->c.seq->data[0].p;
    item->i = ((short*) ptr)[row];
    return IT_int;
}

static ItemTypes Getter_i32 (int row, Item_p item) {
    const char *ptr = item->c.seq->data[0].p;
    item->i = ((const int*) ptr)[row];
    return IT_int;
}

static ItemTypes Getter_i64 (int row, Item_p item) {
    const char *ptr = item->c.seq->data[0].p;
    item->w = ((const int64_t*) ptr)[row];
    return IT_wide;
}

static ItemTypes Getter_f32 (int row, Item_p item) {
    const char *ptr = item->c.seq->data[0].p;
    item->f = ((const float*) ptr)[row];
    return IT_float;
}

static ItemTypes Getter_f64 (int row, Item_p item) {
    const char *ptr = item->c.seq->data[0].p;
    item->d = ((const double*) ptr)[row];
    return IT_double;
}

#endif

static ItemTypes Getter_i16r (int row, Item_p item) {
    const Byte_t *ptr = (const Byte_t*) item->c.seq->data[0].p + row * 2;
#if _BIG_ENDIAN+0
    item->i = (((int8_t) ptr[1]) << 8) | ptr[0];
#else
    item->i = (((int8_t) ptr[0]) << 8) | ptr[1];
#endif
    return IT_int;
}

static ItemTypes Getter_i32r (int row, Item_p item) {
    const char *ptr = (const char*) item->c.seq->data[0].p + row * 4;
    int i;
    for (i = 0; i < 4; ++i)
        item->b[i] = ptr[3-i];
    return IT_int;
}

static ItemTypes Getter_i64r (int row, Item_p item) {
    const char *ptr = (const char*) item->c.seq->data[0].p + row * 8;
    int i;
    for (i = 0; i < 8; ++i)
        item->b[i] = ptr[7-i];
    return IT_wide;
}

static ItemTypes Getter_f32r (int row, Item_p item) {
    Getter_i32r(row, item);
    return IT_float;
}

static ItemTypes Getter_f64r (int row, Item_p item) {
    Getter_i64r(row, item);
    return IT_double;
}

static struct SeqType ST_Get_i0   = { "get_i0"  , Getter_i0   };
static struct SeqType ST_Get_i1   = { "get_i1"  , Getter_i1   };
static struct SeqType ST_Get_i2   = { "get_i2"  , Getter_i2   };
static struct SeqType ST_Get_i4   = { "get_i4"  , Getter_i4   };
static struct SeqType ST_Get_i8   = { "get_i8"  , Getter_i8   };
static struct SeqType ST_Get_i16  = { "get_i16" , Getter_i16  };
static struct SeqType ST_Get_i32  = { "get_i32" , Getter_i32  };
static struct SeqType ST_Get_i64  = { "get_i64" , Getter_i64  };
static struct SeqType ST_Get_i16r = { "get_i16r", Getter_i16r };
static struct SeqType ST_Get_i32r = { "get_i32r", Getter_i32r };
static struct SeqType ST_Get_i64r = { "get_i64r", Getter_i64r };
static struct SeqType ST_Get_f32  = { "get_f32" , Getter_f32  };
static struct SeqType ST_Get_f64  = { "get_f64" , Getter_f64  };
static struct SeqType ST_Get_f32r = { "get_f32r", Getter_f32r };
static struct SeqType ST_Get_f64r = { "get_f64r", Getter_f64r };

SeqType_p PickIntGetter (int bits) {
    switch (bits) {
        default:    Assert(0); /* fall through */
        case 0:     return &ST_Get_i0;
        case 1:     return &ST_Get_i1;
        case 2:     return &ST_Get_i2;
        case 4:     return &ST_Get_i4;
        case 8:     return &ST_Get_i8;
        case 16:    return &ST_Get_i16;
        case 32:    return &ST_Get_i32;
        case 64:    return &ST_Get_i64;
    }
}

SeqType_p FixedGetter (int bytes, int rows, int real, int flip) {
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
        case 16:    return flip ? &ST_Get_i16r : &ST_Get_i16;
        case 32:    return real ? flip ? &ST_Get_f32r : &ST_Get_f32
                                : flip ? &ST_Get_i32r : &ST_Get_i32;
        case 64:    return real ? flip ? &ST_Get_f64r : &ST_Get_f64
                                : flip ? &ST_Get_i64r : &ST_Get_i64;
    }

    return PickIntGetter(bits);
}
