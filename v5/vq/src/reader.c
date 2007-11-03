/* Memory-mapped file and and reader interface */

#include "defs.h"

#include <stdlib.h>

static vq_Type Rgetter_i0 (int row, vq_Item *item) {
    item->o.a.i = 0;
    return VQ_int;
}

static vq_Type Rgetter_i1 (int row, vq_Item *item) {
    const char *ptr = vData(item->o.a.m);
    item->o.a.i = (ptr[row>>3] >> (row&7)) & 1;
    return VQ_int;
}

static vq_Type Rgetter_i2 (int row, vq_Item *item) {
    const char *ptr = vData(item->o.a.m);
    item->o.a.i = (ptr[row>>2] >> 2*(row&3)) & 3;
    return VQ_int;
}

static vq_Type Rgetter_i4 (int row, vq_Item *item) {
    const char *ptr = vData(item->o.a.m);
    item->o.a.i = (ptr[row>>1] >> 4*(row&1)) & 15;
    return VQ_int;
}

static vq_Type Rgetter_i8 (int row, vq_Item *item) {
    const char *ptr = vData(item->o.a.m);
    item->o.a.i = (int8_t) ptr[row];
    return VQ_int;
}

#ifdef VQ_MUSTALIGN

static vq_Type Rgetter_i16 (int row, vq_Item *item) {
    const unsigned char *ptr = 
            (const unsigned char*) vData(item->o.a.m) + row * 2;
#ifdef VQ_BIGENDIAN
    item->o.a.i = (((int8_t) ptr[0]) << 8) | ptr[1];
#else
    item->o.a.i = (((int8_t) ptr[1]) << 8) | ptr[0];
#endif
    return VQ_int;
}

static vq_Type Rgetter_i32 (int row, vq_Item *item) {
    const char *ptr = (const char*) vData(item->o.a.m) + row * 4;
    int i;
    for (i = 0; i < 4; ++i)
        item->b[i] = ptr[i];
    return VQ_int;
}

static vq_Type Rgetter_i64 (int row, vq_Item *item) {
    const char *ptr = (const char*) vData(item->o.a.m) + row * 8;
    int i;
    for (i = 0; i < 8; ++i)
        item->b[i] = ptr[i];
    return VQ_wide;
}

static vq_Type Rgetter_f32 (int row, vq_Item *item) {
    Rgetter_i32(row, item);
    return VQ_float;
}

static vq_Type Rgetter_f64 (int row, vq_Item *item) {
    Rgetter_i64(row, item);
    return VQ_double;
}

#else

static vq_Type Rgetter_i16 (int row, vq_Item *item) {
    const char *ptr = vData(item->o.a.m);
    item->o.a.i = ((short*) ptr)[row];
    return VQ_int;
}

static vq_Type Rgetter_i32 (int row, vq_Item *item) {
    const char *ptr = vData(item->o.a.m);
    item->o.a.i = ((const int*) ptr)[row];
    return VQ_int;
}

static vq_Type Rgetter_i64 (int row, vq_Item *item) {
    const char *ptr = vData(item->o.a.m);
    item->w = ((const int64_t*) ptr)[row];
    return VQ_long;
}

static vq_Type Rgetter_f32 (int row, vq_Item *item) {
    const char *ptr = vData(item->o.a.m);
    item->o.a.f = ((const float*) ptr)[row];
    return VQ_float;
}

static vq_Type Rgetter_f64 (int row, vq_Item *item) {
    const char *ptr = vData(item->o.a.m);
    item->d = ((const double*) ptr)[row];
    return VQ_double;
}

#endif

static vq_Type Rgetter_i16r (int row, vq_Item *item) {
    const unsigned char *ptr = 
            (const unsigned char*) vData(item->o.a.m) + row * 2;
#ifdef VQ_BIGENDIAN
    item->o.a.i = (((int8_t) ptr[1]) << 8) | ptr[0];
#else
    item->o.a.i = (((int8_t) ptr[0]) << 8) | ptr[1];
#endif
    return VQ_int;
}

static vq_Type Rgetter_i32r (int row, vq_Item *item) {
    const char *ptr = (const char*) vData(item->o.a.m) + row * 4;
    int i;
    for (i = 0; i < 4; ++i)
        item->r[i] = ptr[3-i];
    return VQ_int;
}

static vq_Type Rgetter_i64r (int row, vq_Item *item) {
    const char *ptr = (const char*) vData(item->o.a.m) + row * 8;
    int i;
    for (i = 0; i < 8; ++i)
        item->r[i] = ptr[7-i];
    return VQ_long;
}

static vq_Type Rgetter_f32r (int row, vq_Item *item) {
    Rgetter_i32r(row, item);
    return VQ_float;
}

static vq_Type Rgetter_f64r (int row, vq_Item *item) {
    Rgetter_i64r(row, item);
    return VQ_double;
}

static void Rcleaner (Vector v) {
    /* TODO: cleanup map ref */
    FreeVector(v);
}

static Dispatch vget_i0   = { "get_i0"  , 3, 0, 0, Rcleaner, Rgetter_i0   };
static Dispatch vget_i1   = { "get_i1"  , 3, 0, 0, Rcleaner, Rgetter_i1   };
static Dispatch vget_i2   = { "get_i2"  , 3, 0, 0, Rcleaner, Rgetter_i2   };
static Dispatch vget_i4   = { "get_i4"  , 3, 0, 0, Rcleaner, Rgetter_i4   };
static Dispatch vget_i8   = { "get_i8"  , 3, 0, 0, Rcleaner, Rgetter_i8   };
static Dispatch vget_i16  = { "get_i16" , 3, 0, 0, Rcleaner, Rgetter_i16  };
static Dispatch vget_i32  = { "get_i32" , 3, 0, 0, Rcleaner, Rgetter_i32  };
static Dispatch vget_i64  = { "get_i64" , 3, 0, 0, Rcleaner, Rgetter_i64  };
static Dispatch vget_i16r = { "get_i16r", 3, 0, 0, Rcleaner, Rgetter_i16r };
static Dispatch vget_i32r = { "get_i32r", 3, 0, 0, Rcleaner, Rgetter_i32r };
static Dispatch vget_i64r = { "get_i64r", 3, 0, 0, Rcleaner, Rgetter_i64r };
static Dispatch vget_f32  = { "get_f32" , 3, 0, 0, Rcleaner, Rgetter_f32  };
static Dispatch vget_f64  = { "get_f64" , 3, 0, 0, Rcleaner, Rgetter_f64  };
static Dispatch vget_f32r = { "get_f32r", 3, 0, 0, Rcleaner, Rgetter_f32r };
static Dispatch vget_f64r = { "get_f64r", 3, 0, 0, Rcleaner, Rgetter_f64r };

Dispatch* PickIntGetter (int bits) {
    switch (bits) {
        case 0:     return &vget_i0;
        case 1:     return &vget_i1;
        case 2:     return &vget_i2;
        case 4:     return &vget_i4;
        case 8:     return &vget_i8;
        case 16:    return &vget_i16;
        case 32:    return &vget_i32;
        case 64:    return &vget_i64;
    }
    assert(0);
    return 0;
}

Dispatch* FixedGetter (int bytes, int rows, int real, int flip) {
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
    int bits = rows < 8 && bytes < 7 ? widths[rows][bytes] : (bytes<<3) / rows;
    switch (bits) {
        case 16:    return flip ? &vget_i16r : &vget_i16;
        case 32:    return real ? flip ? &vget_f32r : &vget_f32
                                : flip ? &vget_i32r : &vget_i32;
        case 64:    return real ? flip ? &vget_f64r : &vget_f64
                                : flip ? &vget_i64r : &vget_i64;
    }
    return PickIntGetter(bits);
}
