/*  Load views from a Metakit-format memory map.
    $Id$
    This file is part of Vlerq, see src/vlerq.h for full copyright notice. */

/* forward */
static vqView MapSubview (vqMap map, intptr_t offset, vqView meta);

#ifdef VQ_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

static void MapCleaner (vqVec map) {
#ifdef VQ_WIN32
    UnmapViewOfFile(map[1].p);
#else
    munmap(map[1].p, map[1].x.y.i);
#endif
}

static vqDispatch vmmap = { "mmap", 1, 0, MapCleaner };

static vqVec OpenMappedFile (const char *filename) {
    vqVec map;
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

    map = alloc_vec(&vmmap, 2 * sizeof(vqCell));
    map[0].s = map[1].s = (void*) data;
    map[0].x.y.i = map[1].x.y.i = length;
    return map;
}

#if 0
static void BytesCleaner (vqVec map) {
    /* ObjRelease(map[2].p); FIXME: need to release object somehow */
}

static vqDispatch vbytes = { "bytes", 1, 0, BytesCleaner };

static vqVec OpenMappedBytes (const void *data, int length, Object_p ref) {
    vqVec map = alloc_vec(&vbytes, 3 * sizeof(vqCell));
    map[0].s = map[1].s = (void*) data;
    map[0].x.y.i = map[1].x.y.i = length;
    map[2].p = ref; /* ObjRetain(ref); */
    return map;
}
#endif

const char *AdjustMappedFile (vqVec map, int offset) {
    map[0].s += offset;
    map[0].x.y.i -= offset;
    return map[0].s;
}

static vqType Rgetter_i0 (int row, vqCell *cp) {
    VQ_UNUSED(row);
    cp->i = 0;
    return VQ_int;
}
static vqType Rgetter_i1 (int row, vqCell *cp) {
    const char *ptr = (const char*) vData(cp->v);
    cp->i = (ptr[row>>3] >> (row&7)) & 1;
    return VQ_int;
}
static vqType Rgetter_i2 (int row, vqCell *cp) {
    const char *ptr = (const char*) vData(cp->v);
    cp->i = (ptr[row>>2] >> 2*(row&3)) & 3;
    return VQ_int;
}
static vqType Rgetter_i4 (int row, vqCell *cp) {
    const char *ptr = (const char*) vData(cp->v);
    cp->i = (ptr[row>>1] >> 4*(row&1)) & 15;
    return VQ_int;
}
static vqType Rgetter_i8 (int row, vqCell *cp) {
    const char *ptr = (const char*) vData(cp->v);
    cp->i = (int8_t) ptr[row];
    return VQ_int;
}

#ifdef VQ_MUSTALIGN
static vqType Rgetter_i16 (int row, vqCell *cp) {
    const uint8_t *ptr = (const uint8_t*) vData(cp->v) + row * 2;
#ifdef VQ_BIGENDIAN
    cp->i = (((int8_t) ptr[0]) << 8) | ptr[1];
#else
    cp->i = (((int8_t) ptr[1]) << 8) | ptr[0];
#endif
    return VQ_int;
}
static vqType Rgetter_i32 (int row, vqCell *cp) {
    const char *ptr = (const char*) vData(cp->v) + row * 4;
    int i;
    for (i = 0; i < 4; ++i)
        cp->b[i] = ptr[i];
    return VQ_int;
}
static vqType Rgetter_i64 (int row, vqCell *cp) {
    const char *ptr = (const char*) vData(cp->v) + row * 8;
    int i;
    for (i = 0; i < 8; ++i)
        cp->b[i] = ptr[i];
    return VQ_wide;
}
static vqType Rgetter_f32 (int row, vqCell *cp) {
    Rgetter_i32(row, cp);
    return VQ_float;
}
static vqType Rgetter_f64 (int row, vqCell *cp) {
    Rgetter_i64(row, cp);
    return VQ_double;
}
#else
static vqType Rgetter_i16 (int row, vqCell *cp) {
    const char *ptr = (const char*) vData(cp->v);
    cp->i = ((short*) ptr)[row];
    return VQ_int;
}
static vqType Rgetter_i32 (int row, vqCell *cp) {
    const char *ptr = (const char*) vData(cp->v);
    cp->i = ((const int*) ptr)[row];
    return VQ_int;
}
static vqType Rgetter_i64 (int row, vqCell *cp) {
    const char *ptr = (const char*) vData(cp->v);
    cp->w = ((const int64_t*) ptr)[row];
    return VQ_long;
}
static vqType Rgetter_f32 (int row, vqCell *cp) {
    const char *ptr = (const char*) vData(cp->v);
    cp->f = ((const float*) ptr)[row];
    return VQ_float;
}
static vqType Rgetter_f64 (int row, vqCell *cp) {
    const char *ptr = (const char*) vData(cp->v);
    cp->d = ((const double*) ptr)[row];
    return VQ_double;
}
#endif

static vqType Rgetter_i16r (int row, vqCell *cp) {
    const uint8_t *ptr = (const uint8_t*) vData(cp->v) + row * 2;
#ifdef VQ_BIGENDIAN
    cp->i = (((int8_t) ptr[1]) << 8) | ptr[0];
#else
    cp->i = (((int8_t) ptr[0]) << 8) | ptr[1];
#endif
    return VQ_int;
}
static vqType Rgetter_i32r (int row, vqCell *cp) {
    const char *ptr = (const char*) vData(cp->v) + row * 4;
    int i;
    for (i = 0; i < 4; ++i)
        cp->c[i] = ptr[3-i];
    return VQ_int;
}
static vqType Rgetter_i64r (int row, vqCell *cp) {
    const char *ptr = (const char*) vData(cp->v) + row * 8;
    int i;
    for (i = 0; i < 8; ++i)
        cp->c[i] = ptr[7-i];
    return VQ_long;
}
static vqType Rgetter_f32r (int row, vqCell *cp) {
    Rgetter_i32r(row, cp);
    return VQ_float;
}
static vqType Rgetter_f64r (int row, vqCell *cp) {
    Rgetter_i64r(row, cp);
    return VQ_double;
}

static void Rcleaner (vqVec v) {
    vq_release(vOrig(v));
}

static vqDispatch vget_i0   = { "get_i0"  , 3, 0, Rcleaner, Rgetter_i0   };
static vqDispatch vget_i1   = { "get_i1"  , 3, 0, Rcleaner, Rgetter_i1   };
static vqDispatch vget_i2   = { "get_i2"  , 3, 0, Rcleaner, Rgetter_i2   };
static vqDispatch vget_i4   = { "get_i4"  , 3, 0, Rcleaner, Rgetter_i4   };
static vqDispatch vget_i8   = { "get_i8"  , 3, 0, Rcleaner, Rgetter_i8   };
static vqDispatch vget_i16  = { "get_i16" , 3, 0, Rcleaner, Rgetter_i16  };
static vqDispatch vget_i32  = { "get_i32" , 3, 0, Rcleaner, Rgetter_i32  };
static vqDispatch vget_i64  = { "get_i64" , 3, 0, Rcleaner, Rgetter_i64  };
static vqDispatch vget_i16r = { "get_i16r", 3, 0, Rcleaner, Rgetter_i16r };
static vqDispatch vget_i32r = { "get_i32r", 3, 0, Rcleaner, Rgetter_i32r };
static vqDispatch vget_i64r = { "get_i64r", 3, 0, Rcleaner, Rgetter_i64r };
static vqDispatch vget_f32  = { "get_f32" , 3, 0, Rcleaner, Rgetter_f32  };
static vqDispatch vget_f64  = { "get_f64" , 3, 0, Rcleaner, Rgetter_f64  };
static vqDispatch vget_f32r = { "get_f32r", 3, 0, Rcleaner, Rgetter_f32r };
static vqDispatch vget_f64r = { "get_f64r", 3, 0, Rcleaner, Rgetter_f64r };

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

/* ----------------------------------------------- METAKIT UTILITY CODE ----- */

#define MF_Data(x) ((x)->s)
#define MF_Length(x) ((intptr_t) (x)->x.y.p)

static int IsReversedEndian (vqMap map) {
#ifdef VQ_BIGENDIAN
    return *MF_Data(map) == 'J';
#else
    return *MF_Data(map) == 'L';
#endif
}

static intptr_t GetVarInt (const char **nextp) {
    int8_t b;
    intptr_t v = 0;
    do {
        b = *(*nextp)++;
        v = (v << 7) + b;
    } while (b >= 0);
    return v + 128;
}

static intptr_t GetVarPair (const char **nextp) {
    intptr_t n = GetVarInt(nextp);
    if (n > 0 && GetVarInt(nextp) == 0)
        *nextp += n;
    return n;
}

/* ----------------------------------------------- MAPPED TABLE COLUMNS ----- */

static void MappedViewCleaner (vqVec v) {
    const intptr_t *offsets = (void*) v;
    int i, count = vwRows(v);
    for (i = 0; i < count; ++i)
        if ((offsets[i] & 1) == 0)
            vq_decref((void*) offsets[i]);
    vq_decref(vwMeta(v));
    vq_decref(vOrig(v));
}

static vqType MappedViewGetter (int row, vqCell *cp) {
    vqVec v = cp->v;
    intptr_t *offsets = (void*) v;
    
    /* odd means it's a file offset, else it's a cached view pointer */
    if (offsets[row] & 1) {
        intptr_t o = (uintptr_t) offsets[row] >> 1;
        offsets[row] = (intptr_t) vq_incref(MapSubview(vOrig(v), o, vwMeta(v)));
        assert((offsets[row] & 1) == 0);
    }
    
    cp->v = (void*) offsets[row];
    return VQ_view;
}

static vqDispatch mvtab = {
    "mappedview", sizeof(struct vqView_s), 0,
    MappedViewCleaner, MappedViewGetter
};

static vqVec MappedViewCol (vqMap map, int rows, const char **nextp, vqView meta) {
    int r, c, cols, subcols;
    intptr_t colsize, colpos, *offsets;
    const char *next;
    vqVec result;
    
    result = alloc_vec(&mvtab, rows * sizeof(vqView*));
    offsets = (void*) result;
    
    cols = vwRows(meta);
    
    colsize = GetVarInt(nextp);
    colpos = colsize > 0 ? GetVarInt(nextp) : 0;
    next = MF_Data(map) + colpos;
    
    for (r = 0; r < rows; ++r) {
        offsets[r] = 2 * (next - MF_Data(map)) | 1; /* see MappedViewGetter */
        GetVarInt(&next);
        if (cols == 0) {
            intptr_t desclen = GetVarInt(&next);
            meta = DescLenToMeta(next, desclen);
            next += desclen;
        }
        if (GetVarInt(&next) > 0) {
            subcols = vwRows(meta);
            for (c = 0; c < subcols; ++c)
                switch (vq_getInt(meta, c, 1, VQ_nil) & VQ_TYPEMASK) {
                    case VQ_bytes: case VQ_string:
                        if (GetVarPair(&next))
                            GetVarPair(&next);
                }
                GetVarPair(&next);
        }
    }
    
    vwRows(result) = rows;
    vOrig(result) = vq_incref(map);
    vwMeta(result) = vq_incref(cols > 0 ? meta : EmptyMetaView());    
    return result;
}

/* ------------------------------------ MAPPED FIXED-SIZE ENTRY COLUMNS ----- */

static vqVec MappedFixedCol (vqMap map, int rows, const char **nextp, int real) {
    intptr_t bytes = GetVarInt(nextp);
    int colpos = bytes > 0 ? GetVarInt(nextp) : 0;
    vqDispatch *vt = FixedGetter(bytes, rows, real, IsReversedEndian(map));
    vqVec result = alloc_vec(vt, 0);    
    vwRows(result) = rows;
    vData(result) = (void*) (MF_Data(map) + colpos);
    vOrig(result) = vq_incref(map);
    return result;
}

/* ---------------------------------------------- MAPPED STRING COLUMNS ----- */

static void MappedStringCleaner (vqVec v) {
    vq_decref(vwMeta(v));
    vq_decref(vOrig(v));
}

static vqType MappedStringGetter (int row, vqCell *cp) {
    vqVec v = cp->v;
    const intptr_t *offsets = (void*) v;
    const char *data = MF_Data(vOrig(v));

    if (offsets[row] == 0)
        cp->s = "";
    else if (offsets[row] > 0)
        cp->s = data + offsets[row];
    else {
        const char *next = data - offsets[row];
        GetVarInt(&next);
        cp->s = data + GetVarInt(&next);
    }

    return VQ_string;
}

static vqDispatch mstab = {
    "mappedstring", sizeof(struct vqView_s), 0,
    MappedStringCleaner, MappedStringGetter
};

static vqType MappedBytesGetter (int row, vqCell *cp) {
    vqVec v = cp->v;
    const intptr_t *offsets = (void*) v;
    const char *data = MF_Data(vOrig(v));
    
    if (offsets[row] == 0) {
        cp->x.y.i = 0;
        cp->p = 0;
    } else if (offsets[row] > 0) {
        cp->v = vwMeta(v); /* reuse cell to access sizes */
        getcell(row, cp);
        cp->x.y.i = cp->i;
        cp->p = (char*) data + offsets[row];
    } else {
        const char *next = data - offsets[row];
        cp->x.y.i = GetVarInt(&next);
        cp->p = (char*) data + GetVarInt(&next);
    }

    return VQ_bytes;
}

static vqDispatch mbtab = {
    "mappedstring", sizeof(struct vqView_s), 0,
    MappedStringCleaner, MappedBytesGetter
};

static vqVec MappedStringCol (vqMap map, int rows, const char **nextp, int istext) {
    int r;
    intptr_t colsize, colpos, *offsets;
    const char *next, *limit;
    vqVec result, sizes;
    vqCell cell;

    result = alloc_vec(istext ? &mstab : &mbtab, rows * sizeof(void*));
    offsets = (void*) result;

    colsize = GetVarInt(nextp);
    colpos = colsize > 0 ? GetVarInt(nextp) : 0;

    if (colsize > 0) {
        sizes = MappedFixedCol(map, rows, nextp, 0);
        for (r = 0; r < rows; ++r) {
            cell.v = sizes;
            getcell(r, &cell);
            if (cell.i > 0) {
                offsets[r] = colpos;
                colpos += cell.i;
            }
        }
    } else
        sizes = alloc_vec(FixedGetter(0, rows, 0, 0), 0);
        
    vq_incref(sizes);
    
    colsize = GetVarInt(nextp);
    next = MF_Data(map) + (colsize > 0 ? GetVarInt(nextp) : 0);
    limit = next + colsize;
    
    /* negated offsets point to the size/pos pair in the map */
    for (r = 0; next < limit; ++r) {
        r += (int) GetVarInt(&next);
        offsets[r] = MF_Data(map) - next;
        assert(offsets[r] < 0);
        GetVarPair(&next);
    }

    vwRows(result) = rows;
    vOrig(result) = vq_incref(map);
    
    if (istext)
        vq_decref(sizes);
    else
        vwMeta(result) = sizes;

    return result;
}

/* ----------------------------------------------------- TREE TRAVERSAL ----- */

static vqView MapCols (vqMap map, const char **nextp, vqView meta) {
    int c, cols, r, rows;
    vqView result, sub;
    vqVec vec;
    
    rows = GetVarInt(nextp);
    cols = vwRows(meta);
    
    result = vq_new(0, meta);
    vwRows(result) = rows;
    
    if (rows > 0)
        for (c = 0; c < cols; ++c) {
            r = rows;
            switch (vq_getInt(meta, c, 1, VQ_nil) & VQ_TYPEMASK) {
                case VQ_int:
                case VQ_long:
                    vec = MappedFixedCol(map, r, nextp, 0); 
                    break;
                case VQ_float:
                case VQ_double:
                    vec = MappedFixedCol(map, r, nextp, 1);
                    break;
                case VQ_string:
                    vec = MappedStringCol(map, r, nextp, 1); 
                    break;
                case VQ_bytes:
                    vec = MappedStringCol(map, r, nextp, 0);
                    break;
                case VQ_view:
                    sub = vq_getView(meta, c, 2, 0);
                    vec = MappedViewCol(map, r, nextp, sub); 
                    break;
                default:
                    assert(0);
                    return result;
            }
            result[c].v = vq_incref(vec);
        }

    return result;
}

static vqView MapSubview (vqMap map, intptr_t offset, vqView meta) {
    const char *next = MF_Data(map) + offset;
    GetVarInt(&next);
    
    if (vwRows(meta) == 0) {
        intptr_t desclen = GetVarInt(&next);
        meta = DescLenToMeta(next, desclen);
        next += desclen;
    }

    return MapCols(map, &next, meta);
}

static int BigEndianInt32 (const char *p) {
    const uint8_t *up = (const uint8_t*) p;
    return (p[0] << 24) | (up[1] << 16) | (up[2] << 8) | up[3];
}

static vqView MapToView (vqMap map) {
    int i, t[4];
    intptr_t datalen, rootoff;
    
    if (MF_Length(map) <= 24 || *(MF_Data(map) + MF_Length(map) - 16) != '\x80')
        return NULL;
        
    for (i = 0; i < 4; ++i)
        t[i] = BigEndianInt32(MF_Data(map) + MF_Length(map) - 16 + i * 4);
        
    datalen = t[1] + 16;
    rootoff = t[3];

    if (rootoff < 0) {
        const intptr_t mask = 0x7FFFFFFF; 
        datalen = (datalen & mask) + ((intptr_t) ((t[0] & 0x7FF) << 16) << 15);
        rootoff = (rootoff & mask) + (datalen & ~mask);
        /* FIXME: rollover at 2 Gb, prob needs: if (rootoff > datalen) ... */
    }
    
    AdjustMappedFile(map, MF_Length(map) - datalen);
    return MapSubview(map, rootoff, EmptyMetaView());
}

/* -------------------------------------------------- OPERATOR WRAPPERS ----- */

vqView OpenVop (const char *filename) {
    vqMap map = OpenMappedFile(filename);
    return map != NULL ? MapToView(map) : NULL;
}
