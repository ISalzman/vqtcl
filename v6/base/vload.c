/*  Load views from a Metakit-format memory map.
    $Id$
    This file is part of Vlerq, see base/vlerq.h for full copyright notice. */

#include "vq_conf.h"

#if VQ_LOAD_H

/* forward */
static vq_View MapSubview (Vector map, intptr_t offset, vq_View meta);

/* -------------------------------------------------- META DESCRIPTIONS ----- */

static vq_View ParseDesc (char **desc, const char **nbuf, int *tbuf, vq_View *sbuf) {
    char sep, *ptr = *desc;
    const char  **names = nbuf;
    int c, cols = 0, *types = tbuf;
    vq_View result, empty, *subts = sbuf;
    
    empty = EmptyMetaView();
    
    for (;;) {
        const char* s = ptr;
        sbuf[cols] = empty;
        tbuf[cols] = VQ_string;
        
        while (strchr(":,[]", *ptr) == 0)
            ++ptr;
        sep = *ptr;
        *ptr = 0;

        if (sep == '[') {
            ++ptr;
            sbuf[cols] = ParseDesc(&ptr, nbuf+cols, tbuf+cols, sbuf+cols);
            tbuf[cols] = VQ_view;
            sep = *++ptr;
        } else if (sep == ':') {
            tbuf[cols] = CharAsType(*++ptr);
            sep = *++ptr;
        }
        
        nbuf[cols++] = s;
        if (sep != ',')
            break;
            
        ++ptr;
    }
    
    *desc = ptr;

    result = vq_new(cols, vMeta(empty));

    for (c = 0; c < cols; ++c)
        Vq_setMetaRow(result, c, names[c], types[c], subts[c]);
    
    return result;
}
static vq_View DescLenToMeta (const char *desc, int length) {
    int i, bytes, limit = 1;
    void *buffer;
    const char **nbuf;
    int *tbuf;
    vq_View *sbuf, meta;
    char *dbuf;
    
    if (length <= 0)
        return EmptyMetaView();
    
    /* find a hard upper bound for the buffer requirements */
    for (i = 0; i < length; ++i)
        if (desc[i] == ',' || desc[i] == '[')
            ++limit;
    
    bytes = limit * (2 * sizeof(void*) + sizeof(int)) + length + 1;
    buffer = malloc(bytes);
    nbuf = buffer;
    sbuf = (void*) (nbuf + limit);
    tbuf = (void*) (sbuf + limit);
    dbuf = memcpy(tbuf + limit, desc, length);
    dbuf[length] = ']';
    
    meta = ParseDesc(&dbuf, nbuf, tbuf, sbuf);
    
    free(buffer);
    return meta;
}

vq_View AsMetaVop (const char *desc) {
    return DescLenToMeta(desc, strlen(desc));
}

/* ----------------------------------------------- METAKIT UTILITY CODE ----- */

#define MF_Data(x) ((x)->o.a.s)
#define MF_Length(x) ((intptr_t) (x)->o.b.p)

static int IsReversedEndian (Vector map) {
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

static void MappedViewCleaner (Vector v) {
    const intptr_t *offsets = (void*) v;
    int i, count = vCount(v);
    for (i = 0; i < count; ++i)
        if ((offsets[i] & 1) == 0)
            vq_release((void*) offsets[i]);
    vq_release(vMeta(v));
    vq_release(vOrig(v));
    FreeVector(v);
}

static vq_Type MappedViewGetter (int row, vq_Cell *item) {
    Vector v = item->o.a.v;
    intptr_t *offsets = (void*) v;
    
    /* odd means it's a file offset, else it's a cached view pointer */
    if (offsets[row] & 1) {
        intptr_t o = (uintptr_t) offsets[row] >> 1;
        offsets[row] = (intptr_t) vq_retain(MapSubview(vOrig(v), o, vMeta(v)));
        assert((offsets[row] & 1) == 0);
    }
    
    item->o.a.v = (void*) offsets[row];
    return VQ_view;
}

static Dispatch mvtab = {
    "mappedview", 3, 0, 0, MappedViewCleaner, MappedViewGetter
};

static Vector MappedViewCol (Vector map, int rows, const char **nextp, vq_View meta) {
    int r, c, cols, subcols;
    intptr_t colsize, colpos, *offsets;
    const char *next;
    Vector result;
    
    result = AllocVector(&mvtab, rows * sizeof(vq_View*));
    offsets = (void*) result;
    
    cols = vCount(meta);
    
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
            subcols = vCount(meta);
            for (c = 0; c < subcols; ++c)
                switch (Vq_getInt(meta, c, 1, VQ_nil) & VQ_TYPEMASK) {
                    case VQ_bytes: case VQ_string:
                        if (GetVarPair(&next))
                            GetVarPair(&next);
                }
                GetVarPair(&next);
        }
    }
    
    vCount(result) = rows;
    vOrig(result) = vq_retain(map);
    vMeta(result) = vq_retain(cols > 0 ? meta : EmptyMetaView());    
    return result;
}

/* ------------------------------------ MAPPED FIXED-SIZE ENTRY COLUMNS ----- */

static Vector MappedFixedCol (Vector map, int rows, const char **nextp, int real) {
    intptr_t bytes = GetVarInt(nextp);
    int colpos = bytes > 0 ? GetVarInt(nextp) : 0;
    Dispatch *vt = FixedGetter(bytes, rows, real, IsReversedEndian(map));
    Vector result = AllocVector(vt, 0);    
    vCount(result) = rows;
    vData(result) = (void*) (MF_Data(map) + colpos);
    vOrig(result) = vq_retain(map);
    return result;
}

/* ---------------------------------------------- MAPPED STRING COLUMNS ----- */

static void MappedStringCleaner (Vector v) {
    vq_release(vMeta(v));
    vq_release(vOrig(v));
    FreeVector(v);
}

static vq_Type MappedStringGetter (int row, vq_Cell *item) {
    Vector v = item->o.a.v;
    const intptr_t *offsets = (void*) v;
    const char *data = MF_Data(vOrig(v));

    if (offsets[row] == 0)
        item->o.a.s = "";
    else if (offsets[row] > 0)
        item->o.a.s = data + offsets[row];
    else {
        const char *next = data - offsets[row];
        GetVarInt(&next);
        item->o.a.s = data + GetVarInt(&next);
    }

    return VQ_string;
}

static Dispatch mstab = {
    "mappedstring", 3, 0, 0, MappedStringCleaner, MappedStringGetter
};

static vq_Type MappedBytesGetter (int row, vq_Cell *item) {
    Vector v = item->o.a.v;
    const intptr_t *offsets = (void*) v;
    const char *data = MF_Data(vOrig(v));
    
    if (offsets[row] == 0) {
        item->o.b.i = 0;
        item->o.a.p = 0;
    } else if (offsets[row] > 0) {
        item->o.a.v = vMeta(v); /* reuse item to access sizes */
        GetItem(row, item);
        item->o.b.i = item->o.a.i;
        item->o.a.p = (char*) data + offsets[row];
    } else {
        const char *next = data - offsets[row];
        item->o.b.i = GetVarInt(&next);
        item->o.a.p = (char*) data + GetVarInt(&next);
    }

    return VQ_bytes;
}

static Dispatch mbtab = {
    "mappedstring", 3, 0, 0, MappedStringCleaner, MappedBytesGetter
};

static Vector MappedStringCol (Vector map, int rows, const char **nextp, int istext) {
    int r;
    intptr_t colsize, colpos, *offsets;
    const char *next, *limit;
    Vector result, sizes;
    vq_Cell item;

    result = AllocVector(istext ? &mstab : &mbtab, rows * sizeof(void*));
    offsets = (void*) result;

    colsize = GetVarInt(nextp);
    colpos = colsize > 0 ? GetVarInt(nextp) : 0;

    if (colsize > 0) {
        sizes = MappedFixedCol(map, rows, nextp, 0);
        for (r = 0; r < rows; ++r) {
            item.o.a.v = sizes;
            GetItem(r, &item);
            if (item.o.a.i > 0) {
                offsets[r] = colpos;
                colpos += item.o.a.i;
            }
        }
    } else
        sizes = AllocVector(FixedGetter(0, rows, 0, 0), 0);
        
    vq_retain(sizes);
    
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

    vCount(result) = rows;
    vOrig(result) = vq_retain(map);
    
    if (istext)
        vq_release(sizes);
    else
        vMeta(result) = sizes;

    return result;
}

/* ----------------------------------------------------- TREE TRAVERSAL ----- */

static vq_View MapCols (Vector map, const char **nextp, vq_View meta) {
    int c, cols, r, rows;
    vq_View result, sub;
    Vector vec;
    
    rows = GetVarInt(nextp);
    cols = vCount(meta);
    
    result = vq_new(0, meta);
    vCount(result) = rows;
    
    if (rows > 0)
        for (c = 0; c < cols; ++c) {
            r = rows;
            switch (Vq_getInt(meta, c, 1, VQ_nil) & VQ_TYPEMASK) {
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
                    sub = Vq_getView(meta, c, 2, 0);
                    vec = MappedViewCol(map, r, nextp, sub); 
                    break;
                default:
                    assert(0);
                    return result;
            }
            result[c].o.a.v = vq_retain(vec);
        }

    return result;
}

static vq_View MapSubview (Vector map, intptr_t offset, vq_View meta) {
    const char *next = MF_Data(map) + offset;
    GetVarInt(&next);
    
    if (vCount(meta) == 0) {
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

static vq_View MapToView (Vector map) {
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

vq_View OpenVop (const char *filename) {
    Vector map = OpenMappedFile(filename);
    return map != NULL ? MapToView(map) : NULL;
}

#endif
