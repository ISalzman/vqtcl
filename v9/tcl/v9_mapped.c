/* V9 mapped data access as view, using the Metakit file format
   2009-05-08 <jcw@equU4.com> http://opensource.org/licenses/mit-license.php
   $Id$ */
 
#include "v9.priv.h"
#include <string.h>

static V9View MapSubview (Vector map, intptr_t offset, V9View meta); // forward

static V9Item GetOneItem (Vector vp, int row) {
    V9Item item;
    item.c.p = vp;
    assert(Head(vp).type->getter != 0);
    Head(vp).type->getter(row, &item);
    return item;
}

static intptr_t GetVarInt (const char **cp) {
    int8_t b;
    intptr_t v = 0;
    do {
        b = *(*cp)++;
        v = (v << 7) + b;
    } while (b >= 0);
    return v + 128;
}

static intptr_t GetVarPair (const char** cp) {
    intptr_t n = GetVarInt(cp);
    if (n > 0 && GetVarInt(cp) == 0)
        *cp += n;
    return n;
}

static V9View GetMeta (const char** nextp) {
    intptr_t desclen = GetVarInt(nextp);
    *nextp += desclen;
    return V9_DescAsMeta(*nextp - desclen, *nextp);
}

static int IsReversedEndian (Vector map) {
#if _BIG_ENDIAN+0
    return *Address(map) == 'J';
#else
    return *Address(map) == 'L';
#endif
}

static int BigEndianInt32 (const char *p) {
   const uint8_t *up = (const uint8_t*) p;
   return (p[0] << 24) | (up[1] << 16) | (up[2] << 8) | up[3];
}

static Vector MappedFixedCol (Vector map, int nrows, const char **nextp, int isreal) {
    intptr_t colsize = GetVarInt(nextp);
    intptr_t colpos = colsize > 0 ? GetVarInt(nextp) : 0;
    //TODO inline columns: set colpos to next and advance next
    Vector vp = NewSharedVector(map, colpos, colsize);
    Head(vp).count = nrows;
    Head(vp).type = FixedGetter(colsize, nrows, isreal, IsReversedEndian(map));
    return vp;
}

typedef struct MappedTextInfo {
    Vector ovp;     // offset vector pointer, created by MappedViewCol()
    Vector sizes;   // string size vector, entries < for out-of-band entries
    void* unused;
    Vector map;     //XXX must be 4th pointer to overlap SharedVector reference!
} MappedTextInfo;

static void MappedTextCleaner (Vector vp) {
    MappedTextInfo* mti = (void*) vp;
    DecRef(mti->ovp);
    DecRef(mti->sizes);
}

static V9Types MappedStringGetter (int row, V9Item* pitem){
    MappedTextInfo* mti = pitem->c.p;
    const int* offsets = (void*) mti->ovp;
    char *data = (void*) Address(mti->map); // drop const
    
    if (offsets[row] == 0)
        pitem->s = "";
    else if (offsets[row] > 0)
        pitem->s = data + offsets[row];
    else {
        const char *next = data - offsets[row];
        if (GetVarInt(&next) > 0)
            pitem->s = data + GetVarInt(&next);
        else
            pitem->s = "";
    }

    return V9T_string;
}

static V9Types MappedBytesGetter (int row, V9Item* pitem){
    MappedTextInfo* mti = pitem->c.p;
    const int* offsets = (void*) mti->ovp;
    char *data = (void*) Address(mti->map); // drop const
    
    pitem->c.i = GetOneItem(mti->sizes, row).i;
    
    if (offsets[row] >= 0)
        pitem->c.p = data + offsets[row];
    else {
        const char *next = data - offsets[row];
        pitem->c.i = GetVarInt(&next);
        pitem->c.p = data + GetVarInt(&next);
    }

    return V9T_bytes;
}

static struct VectorType vtMappedString = {
    "mappedString", 4, MappedTextCleaner, MappedStringGetter,
};

static struct VectorType vtMappedBytes = {
    "mappedBytes", 4, MappedTextCleaner, MappedBytesGetter,
};

static Vector MappedTextCol (Vector map, int nrows, const char **nextp, int istext) {
    Vector ovp = NewInlineVector(nrows * sizeof(int));
    Head(ovp).count = nrows;
    Head(ovp).type = PickIntGetter(32); //XXX not ready for files > 2 Gb
    int* offsets = (void*) ovp;

    intptr_t colsize = GetVarInt(nextp);
    intptr_t colpos = colsize > 0 ? GetVarInt(nextp) : 0;

    Vector sizes;
    int r;

    if (colsize > 0) {
        sizes = MappedFixedCol(map, nrows, nextp, 0);
        for (r = 0; r < nrows; ++r) {
            int len = GetOneItem(sizes, r).i;
            if (len > 0) {
                offsets[r] = colpos;
                colpos += len;
            }
        }
    } else {
        sizes = NewDataVec(V9T_int, 0);
        Head(sizes).count = nrows;
        Head(sizes).type = PickIntGetter(0);
    }

    intptr_t memosize = GetVarInt(nextp);
    intptr_t memopos = memosize > 0 ? GetVarInt(nextp) : 0;
    const char* next = Address(map) + memopos;
    const char* limit = next + memosize;
    
    /* negated offsets point to the size/pos pair in the map */
    for (r = 0; next < limit; ++r) {
        r += (int) GetVarInt(&next);
        offsets[r] = Address(map) - next; /* always < 0 */
        GetVarPair(&next);
    }
    
    Vector vp = NewSharedVector(map, colpos, colsize);
    Head(vp).count = nrows;
    Head(vp).type = istext ? &vtMappedString : &vtMappedBytes;

    MappedTextInfo* mti = (void*) vp;
    mti->ovp = IncRef(ovp);
    mti->sizes = IncRef(sizes);
    assert(mti->map == map);

    return vp;
}

typedef struct MappedViewInfo {
    Vector ovp;     // offset vector pointer, created by MappedViewCol()
    V9View cache;   // merely holds a reference to the last returned subview
    V9View meta;    // meta-view of the subviews in this view column
    Vector map;     //XXX must be 4th pointer to overlap SharedVector reference!
} MappedViewInfo;

static void MappedViewCleaner (Vector vp) {
    MappedViewInfo* mvi = (void*) vp;
    DecRef(mvi->ovp);
    V9_Release(mvi->cache);
    V9_Release(mvi->meta);
}

static V9Types MappedViewGetter (int row, V9Item* pitem) {
    MappedViewInfo* mvi = pitem->c.p;
    const int* offsets = (void*) mvi->ovp;
    V9View v = MapSubview(mvi->map, offsets[row], mvi->meta);
    V9_Release(mvi->cache);
    pitem->v = mvi->cache = V9_AddRef(v);
    return V9T_view;
}

static struct VectorType vtMappedView = {
    "mappedView", 4, MappedViewCleaner, MappedViewGetter,
};

static Vector MappedViewCol (Vector map, int nrows, const char **nextp, V9View meta) {
    Vector ovp = NewInlineVector(nrows * sizeof(int));
    Head(ovp).count = nrows;
    Head(ovp).type = PickIntGetter(32); //XXX not ready for files > 2 Gb
    int* offsets = (void*) ovp;
    
    int r, c, ncols = Head(meta).count;
    
    intptr_t colsize = GetVarInt(nextp);
    intptr_t colpos = colsize > 0 ? GetVarInt(nextp) : 0;
    const char* next = Address(map) + colpos;
    
    for (r = 0; r < nrows; ++r) {
        offsets[r] = next - Address(map);
        GetVarInt(&next);
        if (ncols == 0)
            meta = GetMeta(&next);
        if (GetVarInt(&next) > 0) {
            int subcols = Head(meta).count;
            for (c = 0; c < subcols; ++c)
                switch (V9_GetInt(meta, c, 1)) {
                    case V9T_string: case V9T_bytes:
                        if (GetVarPair(&next))
                            GetVarPair(&next);
                }
                GetVarPair(&next);
        }
    }
    
    Vector vp = NewSharedVector(map, colpos, colsize);
    Head(vp).count = nrows;
    Head(vp).type = &vtMappedView;

    MappedViewInfo* mvi = (void*) vp;
    mvi->ovp = IncRef(ovp);
    mvi->meta = V9_AddRef(meta);
    assert(mvi->map == map);

    return vp;
}

static V9View MapSubview (Vector map, intptr_t offset, V9View meta) {
    const char* next = Address(map) + offset;
    GetVarInt(&next);
    
    if (Head(meta).count == 0)
        meta = GetMeta(&next);

    int nrows = (int) GetVarInt(&next), ncols = Head(meta).count;
    if (ncols == 0)
        return V9_NewDataView(meta, nrows);

    V9View v = V9_NewDataView(meta, 0);
    Head(v).count = nrows;
    if (nrows > 0) {
        Vector vp;
        int c;
        for (c = 0; c < ncols; ++c) {
            switch (V9_GetInt(meta, c, 1)) {
                case V9T_int:
                case V9T_long:
                    vp = MappedFixedCol(map, nrows, &next, 0); 
                    break;
                case V9T_float:
                case V9T_double:
                    vp = MappedFixedCol(map, nrows, &next, 1);
                    break;
                case V9T_string:
                    vp = MappedTextCol(map, nrows, &next, 1); 
                    break;
                case V9T_bytes:
                    vp = MappedTextCol(map, nrows, &next, 0);
                    break;
                case V9T_view:
                    vp = MappedViewCol(map, nrows, &next,
                                                    V9_GetView(meta, c, 2)); 
                    break;
                default:
                    assert(0);
                    return 0;
            }
            v->col[c].c.p = IncRef(vp);
        }
    }
    return v;
}

static struct VectorType vtMetakit = { "metakit" };

static V9View MappedToView (Vector map) { 
    if (Head(map).count <= 24 ||
            *(Address(map) + Head(map).count - 16) != '\x80')
        return 0;
 
    int i, t[4];
    for (i = 0; i < 4; ++i)
        t[i] = BigEndianInt32(Address(map) + Head(map).count - 16 + i * 4);
 
    intptr_t datalen = t[1] + 16;
    intptr_t rootoff = t[3];
 
    if (rootoff < 0) {
        const intptr_t mask = 0x7FFFFFFF; 
        datalen = (datalen & mask) + ((intptr_t) ((t[0] & 0x7FF) << 16) << 15);
        rootoff = (rootoff & mask) + (datalen & ~mask);
        /* FIXME: rollover at 2 Gb, prob needs: if (rootoff > datalen) ... */
    }
 
    Vector vp = NewSharedVector(map, Head(map).count - datalen, datalen);
    Head(vp).type = &vtMetakit;
    return MapSubview(vp, rootoff, EmptyMeta());
}

static struct VectorType vtMapped = { "mappedData" };

V9View V9_MapDataAsView (const void* ptr, int len) {
    Vector vp = NewInlineVector(len);
    Head(vp).type = &vtMapped;
    memcpy(vp, ptr, len);
    return MappedToView(vp);
}
