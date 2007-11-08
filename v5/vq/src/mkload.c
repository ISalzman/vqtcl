/* Load tables from a Metakit-format memory map */

#include "defs.h"

#if VQ_MOD_MKLOAD

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
/* forward */
static vq_Table MapSubtable (Vector map, intptr_t offset, vq_Table meta);

#pragma mark - META DESCRIPTIONS -

static vq_Table ParseDesc (char **desc, const char **nbuf, int *tbuf, vq_Table *sbuf) {
    char sep, *ptr = *desc;
    const char  **names = nbuf;
    int c, cols = 0, *types = tbuf;
    vq_Table result, empty, *subts = sbuf;
    
    empty = EmptyMetaTable();
    
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
            tbuf[cols] = VQ_table;
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

    result = vq_new(vMeta(empty), cols);

    for (c = 0; c < cols; ++c) {
        Vq_setString(result, c, 0, names[c]);
        Vq_setInt(result, c, 1, types[c]);
        Vq_setTable(result, c, 2, subts[c]);
    }
    
    return result;
}
vq_Table DescToMeta (const char *desc, int length) {
    int i, bytes, limit = 1;
    void *buffer;
    const char **nbuf;
    int *tbuf;
    vq_Table *sbuf, meta;
    char *dbuf;
    
    if (length < 0)
        length = strlen(desc);
    if (length == 0)
        return EmptyMetaTable();
    
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

#pragma mark - METAKIT UTILITY CODE -

#define MF_Data(x) ((x)->o.a.s)
#define MF_Length(x) ((intptr_t) (x)->o.b.p)

static int IsReversedEndian (Vector map) {
#ifdef VQ_BIG_ENDIAN
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

#pragma mark - METAKIT DATA READER -

static void MappedTableCleaner (Vector v) {
    const vq_Table *subs = (void*) v;
    int i, count = vCount(v);
    for (i = 0; i < count; ++i)
        vq_release(subs[i]);
    vq_release(vExtra(v));
    vq_release(vMeta(v));
    vq_release(vOrig(v));
    FreeVector(v);
}

static vq_Type MappedTableGetter (int row, vq_Item *item) {
    Vector v = item->o.a.m;
    vq_Table *subs = (void*) v;
    
    if (subs[row] == NULL) {
        const intptr_t *offsets = vData(v);
        subs[row] = vq_retain(MapSubtable(vOrig(v), offsets[row], vMeta(v)));
    }
    
    item->o.a.m = subs[row];
    return VQ_table;
}

static Dispatch mvtab = {
    "mappedtable", 3, 0, 0, MappedTableCleaner, MappedTableGetter
};

static Vector MappedTableCol (Vector map, int rows, const char **nextp, vq_Table meta) {
    int r, c, cols, subcols;
    intptr_t colsize, colpos, *offsets;
    const char *next;
    Vector offvec, result;
    
    offvec = AllocDataVec(VQ_int, rows);
    offsets = (void*) offvec;
    
    cols = vCount(meta);
    
    colsize = GetVarInt(nextp);
    colpos = colsize > 0 ? GetVarInt(nextp) : 0;
    next = MF_Data(map) + colpos;
    
    for (r = 0; r < rows; ++r) {
        offsets[r] = next - MF_Data(map);
        GetVarInt(&next);
        if (cols == 0) {
            intptr_t desclen = GetVarInt(&next);
            meta = DescToMeta(next, desclen);
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
    
    result = AllocVector(&mvtab, rows * sizeof(vq_Table*));
    vCount(result) = rows;
    vExtra(result) = vq_retain(offvec);
    vData(result) = offsets;
    vOrig(result) = vq_retain(map);
    vMeta(result) = vq_retain(cols > 0 ? meta : EmptyMetaTable());    
    /* TODO: could combine subtable cache and offsets vector */
    return result;
}

static Vector MappedFixedCol (Vector map, int rows, const char **nextp, int real) {
    intptr_t bytes = GetVarInt(nextp);
    int colpos = bytes > 0 ? GetVarInt(nextp) : 0;
    Dispatch *vtab = FixedGetter(bytes, rows, real, IsReversedEndian(map));
    Vector result = AllocVector(vtab, 0);    
    vCount(result) = rows;
    vData(result) = (void*) (MF_Data(map) + colpos);
    vOrig(result) = vq_retain(map);
    return result;
}

static void MappedStringCleaner (Vector v) {
    vq_release(vExtra(v));
    vq_release(vMeta(v));
    vq_release(vOrig(v));
    FreeVector(v);
}

static vq_Type MappedStringGetter (int row, vq_Item *item) {
    Vector v = item->o.a.m;
    const intptr_t *offsets = vData(v);
    const char *data = MF_Data(vOrig(v));

    if (offsets[row] == 0)
        item->o.a.s = "";
    else if (offsets[row] > 0)
        item->o.a.s = data + offsets[row];
    else {
        const char *next = data - offsets[row];
        if (GetVarInt(&next) > 0)
            item->o.a.s = data + GetVarInt(&next);
        else
            item->o.a.s = "";
    }

    return VQ_string;
}

static Dispatch mstab = {
    "mappedstring", 3, 0, 0, MappedStringCleaner, MappedStringGetter
};

static vq_Type MappedBytesGetter (int row, vq_Item *itemp) {
    Vector v = itemp->o.a.m;
    const intptr_t *offsets = vData(v);
    const char *data = MF_Data(vOrig(v));
    itemp->o.a.m = vMeta(v);
    GetItem(row, itemp);
    itemp->o.b.i = itemp->o.a.i;
    
    if (offsets[row] >= 0)
        itemp->o.a.p = (char*) data + offsets[row];
    else {
        const char *next = data - offsets[row];
        itemp->o.b.i = GetVarInt(&next);
        itemp->o.a.p = (char*) data + GetVarInt(&next);
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
    Vector offvec, result, sizes;
    vq_Item item;

    offvec = AllocDataVec(VQ_int, rows);
    offsets = (void*) offvec;

    colsize = GetVarInt(nextp);
    colpos = colsize > 0 ? GetVarInt(nextp) : 0;

    if (colsize > 0) {
        sizes = MappedFixedCol(map, rows, nextp, 0);
        for (r = 0; r < rows; ++r) {
            item.o.a.m = sizes;
            GetItem(r, &item);
            if (item.o.a.i > 0) {
                offsets[r] = colpos;
                colpos += item.o.a.i;
            }
        }
    } else
        sizes = AllocVector(FixedGetter(0, rows, 0, 0), 0);
    
    colsize = GetVarInt(nextp);
    next = MF_Data(map) + (colsize > 0 ? GetVarInt(nextp) : 0);
    limit = next + colsize;
    
    /* negated offsets point to the size/pos pair in the map */
    for (r = 0; next < limit; ++r) {
        r += (int) GetVarInt(&next);
        offsets[r] = MF_Data(map) - next; /* always < 0 */
        GetVarPair(&next);
    }

    result = AllocVector(istext ? &mstab : &mbtab, 0);
    vCount(result) = rows;
    vExtra(result) = vq_retain(offvec);
    vData(result) = offsets;
    vOrig(result) = vq_retain(map);
    if (istext)
        vq_release(sizes);
    else
        vMeta(result) = vq_retain(sizes);

    return result;
}

static vq_Table MapCols (Vector map, const char **nextp, vq_Table meta) {
    int c, cols, r, rows;
    vq_Table result, sub;
    Vector vec;
    
    rows = GetVarInt(nextp);
    cols = vCount(meta);
    
    result = vq_new(meta, 0);
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
                case VQ_table:
                    sub = Vq_getTable(meta, c, 2, 0);
                    vec = MappedTableCol(map, r, nextp, sub); 
                    break;
                default:
                    assert(0);
                    return result;
            }
            result[c].o.a.m = vq_retain(vec);
        }

    return result;
}

static vq_Table MapSubtable (Vector map, intptr_t offset, vq_Table meta) {
    const char *next = MF_Data(map) + offset;
    GetVarInt(&next);
    
    if (vCount(meta) == 0) {
        intptr_t desclen = GetVarInt(&next);
        meta = DescToMeta(next, desclen);
        next += desclen;
    }

    return MapCols(map, &next, meta);
}

static int BigEndianInt32 (const char *p) {
    const uint8_t *up = (const uint8_t*) p;
    return (p[0] << 24) | (up[1] << 16) | (up[2] << 8) | up[3];
}

vq_Table MapToTable (Vector map) {
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
    return MapSubtable(map, rootoff, EmptyMetaTable());
}

#pragma mark - OPERATOR WRAPPERS -

vq_Type Desc2MetaCmd_S (vq_Item a[]) {
    a->o.a.m = DescToMeta(a[0].o.a.s, -1);
    return VQ_table;
}

vq_Type OpenCmd_S (vq_Item a[]) {
    Vector map = OpenMappedFile(a[0].o.a.s);
    if (map == 0)
        return VQ_nil;
    a->o.a.m = MapToTable(map);
    return VQ_table;
}

#endif
