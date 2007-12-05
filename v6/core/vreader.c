/*  Memory-mapped file and and reader interface.
    $Id$
    This file is part of Vlerq, see lvq.h for the full copyright notice.  */

#include "vdefs.h"

#if VQ_MOD_LOAD

#ifdef VQ_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

static void MapCleaner (Vector map) {
#ifdef VQ_WIN32
    UnmapViewOfFile(map[1].o.a.p);
#else
    munmap(map[1].o.a.p, map[1].o.b.i);
#endif
    FreeVector(map);
}
static Dispatch vmmap = { "mmap", 1, 0, 0, MapCleaner };
Vector OpenMappedFile (const char *filename) {
    Vector map;
    const char *data = NULL;
    intptr_t length = -1;

#ifdef VQ_WIN32
    {
        DWORD n;
        HANDLE h, f = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, 0,
                                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (f != INVALID_HANDLE_VALUE) {
            h = CreateFileMapping(f, 0, PAGE_READONLY, 0, 0, 0);
            if (h != INVALID_HANDLE_VALUE) {
                n = GetFileSize(f, 0);
                data = MapViewOfFile(h, FILE_MAP_READ, 0, 0, n);
                if (data != NULL)
                    length = n;
                CloseHandle(h);
            }
            CloseHandle(f);
        }
    }
#else
    {
        struct stat sb;
        int fd = open(filename, O_RDONLY);
        if (fd != -1) {
            if (fstat(fd, &sb) == 0) {
                data = mmap(0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
                if (data != MAP_FAILED)
                    length = sb.st_size;
            }
            close(fd);
        }
    }
#endif

    if (length < 0)
        return NULL;

    map = AllocVector(&vmmap, 2 * sizeof(vq_Item));
    map[0].o.a.s = map[1].o.a.s = (void*) data;
    map[0].o.b.i = map[1].o.b.i = length;
    return map;
}
static void BytesCleaner (Vector map) {
    /* ObjDecRef(map[2].o.a.p); FIXME: cleanup! */
    FreeVector(map);
}
static Dispatch vbytes = { "bytes", 1, 0, 0, BytesCleaner };
Vector OpenMappedBytes (const void *data, int length, Object_p ref) {
    Vector map = AllocVector(&vbytes, 3 * sizeof(vq_Item));
    map[0].o.a.s = map[1].o.a.s = (void*) data;
    map[0].o.b.i = map[1].o.b.i = length;
    map[2].o.a.p = ref; /* ObjIncRef(ref); */
    return map;
}
const char *AdjustMappedFile (Vector map, int offset) {
    map[0].o.a.s += offset;
    map[0].o.b.i -= offset;
    return map[0].o.a.s;
}

static vq_Type Rgetter_i0 (int row, vq_Item *item) {
    VQ_UNUSED(row);
    item->o.a.i = 0;
    return VQ_int;
}
static vq_Type Rgetter_i1 (int row, vq_Item *item) {
    const char *ptr = vData(item->o.a.v);
    item->o.a.i = (ptr[row>>3] >> (row&7)) & 1;
    return VQ_int;
}
static vq_Type Rgetter_i2 (int row, vq_Item *item) {
    const char *ptr = vData(item->o.a.v);
    item->o.a.i = (ptr[row>>2] >> 2*(row&3)) & 3;
    return VQ_int;
}
static vq_Type Rgetter_i4 (int row, vq_Item *item) {
    const char *ptr = vData(item->o.a.v);
    item->o.a.i = (ptr[row>>1] >> 4*(row&1)) & 15;
    return VQ_int;
}
static vq_Type Rgetter_i8 (int row, vq_Item *item) {
    const char *ptr = vData(item->o.a.v);
    item->o.a.i = (int8_t) ptr[row];
    return VQ_int;
}

#ifdef VQ_MUSTALIGN
static vq_Type Rgetter_i16 (int row, vq_Item *item) {
    const uint8_t *ptr = (const uint8_t*) vData(item->o.a.v) + row * 2;
#ifdef VQ_BIGENDIAN
    item->o.a.i = (((int8_t) ptr[0]) << 8) | ptr[1];
#else
    item->o.a.i = (((int8_t) ptr[1]) << 8) | ptr[0];
#endif
    return VQ_int;
}
static vq_Type Rgetter_i32 (int row, vq_Item *item) {
    const char *ptr = (const char*) vData(item->o.a.v) + row * 4;
    int i;
    for (i = 0; i < 4; ++i)
        item->b[i] = ptr[i];
    return VQ_int;
}
static vq_Type Rgetter_i64 (int row, vq_Item *item) {
    const char *ptr = (const char*) vData(item->o.a.v) + row * 8;
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
    const char *ptr = vData(item->o.a.v);
    item->o.a.i = ((short*) ptr)[row];
    return VQ_int;
}
static vq_Type Rgetter_i32 (int row, vq_Item *item) {
    const char *ptr = vData(item->o.a.v);
    item->o.a.i = ((const int*) ptr)[row];
    return VQ_int;
}
static vq_Type Rgetter_i64 (int row, vq_Item *item) {
    const char *ptr = vData(item->o.a.v);
    item->w = ((const int64_t*) ptr)[row];
    return VQ_long;
}
static vq_Type Rgetter_f32 (int row, vq_Item *item) {
    const char *ptr = vData(item->o.a.v);
    item->o.a.f = ((const float*) ptr)[row];
    return VQ_float;
}
static vq_Type Rgetter_f64 (int row, vq_Item *item) {
    const char *ptr = vData(item->o.a.v);
    item->d = ((const double*) ptr)[row];
    return VQ_double;
}
#endif

static vq_Type Rgetter_i16r (int row, vq_Item *item) {
    const uint8_t *ptr = (const uint8_t*) vData(item->o.a.v) + row * 2;
#ifdef VQ_BIGENDIAN
    item->o.a.i = (((int8_t) ptr[1]) << 8) | ptr[0];
#else
    item->o.a.i = (((int8_t) ptr[0]) << 8) | ptr[1];
#endif
    return VQ_int;
}
static vq_Type Rgetter_i32r (int row, vq_Item *item) {
    const char *ptr = (const char*) vData(item->o.a.v) + row * 4;
    int i;
    for (i = 0; i < 4; ++i)
        item->r[i] = ptr[3-i];
    return VQ_int;
}
static vq_Type Rgetter_i64r (int row, vq_Item *item) {
    const char *ptr = (const char*) vData(item->o.a.v) + row * 8;
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
    vq_release(vOrig(v));
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

#endif
