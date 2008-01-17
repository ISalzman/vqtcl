/*  Load views from a Metakit-format memory map.
    $Id$
    This file is part of Vlerq, see lvq/vlerq.h for full copyright notice. */

#include "vlerq.h"
#include "defs.h"

/* forward */
static vqView MapSubview (vqMap map, intptr_t offset, vqView meta);

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
    
    result = AllocVector(&mvtab, rows * sizeof(vqView*));
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
    vqVec result = AllocVector(vt, 0);    
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

    result = AllocVector(istext ? &mstab : &mbtab, rows * sizeof(void*));
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
        sizes = AllocVector(FixedGetter(0, rows, 0, 0), 0);
        
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
