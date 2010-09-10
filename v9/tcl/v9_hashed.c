/* V9 hashed operations
   2009-05-08 <jcw@equU4.com> http://opensource.org/licenses/mit-license.php
   $Id$ */
 
#include "v9.priv.h"
#include <string.h>

static V9View (HashValues) (V9View view); /* forward */
    
typedef struct HashInfo *HashInfo_p;

typedef struct HashInfo {
    V9View  view;          /* input view */
    V9View  vim, vhv, vha; /* views for the three following columns */
    int    *imap;          /* map of unique rows */
    int    *hvec;          /* hash probe vector (with prime & fill at end) */
    int    *hashes;        /* hash values, one per view row */
} HashInfo;

#define hiPrime(hi) (hi)->hvec[Head((hi)->vhv).count-2]
#define hiFill(hi)  (hi)->hvec[Head((hi)->vhv).count-1]

static V9View NewPosView (int rows) {
    return V9_NewDataView(V9_DescAsMeta(":I", 0), rows);
}

static int StringHash (const char *s, int n) {
    /* similar to Python's stringobject.c */
    int i, h;
    if (n <= 0)
        return 0;
    h = (*s * 0xFF) << 7;
    for (i = 1; i < n; ++i) /* TODO: start at 1 or at 0? */
        h = (1000003 * h) ^ s[i];
    return h ^ i;
}

static V9View HashCol (V9Item column) {
    int i, count, *data;
    V9Item item;
    V9View v;

    count = Head(column.c.p).count;
    v = NewPosView(count);
    data = v->col[0].p;

    for (i = 0; i < count; ++i) {
        item = column;
        switch (GetItem(i, &item)) {
            case V9T_int:
                data[i] = item.i;
                break;
            case V9T_long:
                data[i] = item.i ^ item.c.i;
                break;
            case V9T_float:
                data[i] = item.i;
                break;
            case V9T_double:
                data[i] = item.i ^ item.c.i;
                break;
            case V9T_string:
                data[i] = StringHash(item.s, strlen(item.s));
                break;
            case V9T_bytes:
                data[i] = StringHash(item.s, item.c.i);
                break;
            case V9T_view: {
                V9View hashes = V9_AddRef(HashValues(item.v));
                const int *hvec = hashes->col[0].p;
                int j, hcount = Head(hashes).count, hval = 0;
                for (j = 0; j < hcount; ++j)
                    hval ^= hvec[j];        
                data[i] = hval ^ hcount;
                V9_Release(hashes);
                break;
            }
            default: assert(0);
        }
    }

    return v;
}

#if 0
vqType HashColCmd_SO (V9Item args[]) {
    vqType type = CharAsItemType(args[0].s[0]);
    V9Item column = CoerceColumn(type, args[1].o);
    if (column.p == 0)
        return V9T_nil;        
    *args = HashCol(column);
    return IT_column;
}
#endif

static int RowHash (V9View view, int row) {
    int c, ncols = Head(view->meta).count, hash = 0;

    for (c = 0; c < ncols; ++c) {
        V9Item item = view->col[c];
        switch (GetItem(row, &item)) {

            case V9T_int:
                hash ^= item.i;
                break;

            case V9T_string:
                hash ^= StringHash(item.s, strlen(item.s));
                break;

            case V9T_bytes:
                hash ^= StringHash(item.s, item.c.i);
                break;

            default: {
                /* TODO: ugly, using temp col with single int to map to view */
                V9View tview = NewPosView(1), vhv;
                ((int*) tview)[0] = row;
                vhv = V9_AddRef(HashValues(V9_RowMapView(view, tview)));
                hash = V9_GetInt(vhv, 0, 0);
                V9_Release(vhv);
                break;
            }
        }
    }
    
    return hash;
}

static void XorWithIntCol (int *src, int *dest) {
    int i, count = Head(src).count;

    for (i = 0; i < count; ++i)
        dest[i] ^= src[i];
}

static V9View HashValues (V9View view) {
    int c, *ivec, nr = Head(view).count, nc = Head(view->meta).count;
    V9View t;

    if (nc == 0 || nr == 0)
        return NewPosView(nr);
        
    t = HashCol(view->col[0]);
    ivec = t->col[0].p;
    for (c = 1; c < nc; ++c) {
        V9View u = V9_AddRef(HashCol(view->col[c]));
        int *auxcol = u->col[0].p;
        /* TODO: get rid of the separate xor step by xoring in HashCol */
        XorWithIntCol(auxcol, ivec);
        V9_Release(u);
    }
    return t;
}

static int HashFind (V9View keyview, int keyrow, int keyhash, HashInfo_p data) {
    int probe, datarow, mask, step, prime;

    mask = Head(data->vhv).count - 3;
    prime = hiPrime(data);
    probe = ~keyhash & mask;

    step = (keyhash ^ (keyhash >> 3)) & mask;
    if (step == 0)
        step = mask;

    for (;;) {
        probe = (probe + step) & mask;
        if (data->hvec[probe] == 0)
            break;

        datarow = data->imap[data->hvec[probe]-1];
        if (keyhash == data->hashes[datarow] &&
                V9_RowEqual(keyview, keyrow, data->view, datarow))
            return data->hvec[probe] - 1;

        step <<= 1;
        if (step > mask)
            step ^= prime;
    }

    if (keyview == data->view) {
        data->hvec[probe] = hiFill(data) + 1;
        data->imap[hiFill(data)++] = keyrow;
    }

    return -1;
}

static int Log2bits (int n) {
    int bits = 0;
    while ((1 << bits) < n)
        ++bits;
    return bits;
}

static V9View HashVector (int rows) {
    static char slack [] = {
        0, 0, 3, 3, 3, 5, 3, 3, 29, 17, 9, 5, 83, 27, 43, 3,
        45, 9, 39, 39, 9, 5, 3, 33, 27, 9, 71, 39, 9, 5, 83, 0
    };

    V9View v;
    int bits = Log2bits((4 * rows) / 3), *iptr, n;
    if (bits < 2)
        bits = 2;
    n = 1 << bits;
    v = NewPosView(n+2);
    iptr = v->col[0].p;
    iptr[n] = n + slack[Log2bits(n-1)]; /* prime */
    return v;
}

static void InitHashInfo (HashInfo_p info, V9View view, V9View vim, V9View vhv, V9View vha) {
    info->view = view;
    info->vim = V9_AddRef(vim);
    info->imap = vim->col[0].p;
    info->vhv = V9_AddRef(vhv);
    info->hvec = vhv->col[0].p;
    info->vha = V9_AddRef(vha);
    info->hashes = vha->col[0].p;
    
    /* FIXME: cleanup! */
}

static void FillHashInfo (HashInfo_p info, V9View view) {
    int r, rows = Head(view).count;

    InitHashInfo(info, view, NewPosView(rows), HashVector(rows),
                                                            HashValues(view));

    for (r = 0; r < rows; ++r)
        HashFind(view, r, info->hashes[r], info);

    /* TODO: reclaim unused entries at end of map */
    Head(info->vim).count = Head(info->imap).count = hiFill(info);
}

static void ChaseLinks (HashInfo_p info, int count, const int *hmap, const int *lmap, int groups) {
    int *smap = info->hvec, *gmap = info->hashes;
    
    while (--groups >= 0) {
        int head = hmap[groups] - 1;
        smap[groups] = count;
        while (head >= 0) {
            gmap[--count] = head;
            head = lmap[head];
        }
    }
    /* assert(count == 0); */
}

static void FillGroupInfo (HashInfo_p info, V9View view) {
    int g, r, rows, *hmap, *lmap;
    V9View vhm, vlm;
    
    rows = Head(view).count;
    vhm = NewPosView(rows);
    hmap = vhm->col[0].p;
    vlm = NewPosView(rows);
    lmap = vlm->col[0].p;

    InitHashInfo(info, view, NewPosView(rows), HashVector(rows),
                                                            HashValues(view));

    for (r = 0; r < rows; ++r) {
        g = HashFind(view, r, info->hashes[r], info);
        if (g < 0)
            g = hiFill(info) - 1;
        lmap[r] = hmap[g] - 1;
        hmap[g] = r + 1;
    }
    
    /* TODO: reclaim unused entries at end of map and hvec */
    Head(info->vim).count = Head(info->imap).count = hiFill(info);
    
    ChaseLinks(info, rows, hmap, lmap, hiFill(info));

    Head(info->vhv).count = Head(info->hvec).count = Head(info->imap).count;
    
    /* TODO: could release headmap and linkmap but that's a no-op here */
    
    /*  There's probably an opportunity to reduce space usage further,
        since the grouping map points to the starting row of each group:
            map[i] == gmap[smap[i]]
        Perhaps "map" (which starts out with #rows entries) can be re-used
        to append the gmap entries (if we can reduce it by #groups items).
        Or just release map[x] for grouping views, and use gmap[smap[x]].
    */
}

#if 0
static int h_groupinfo (vqEnv env) {
    HashInfo info;
    V9View v = check_view(env, 1);
    FillGroupInfo(&info, v);
    push_view(info.vim);
    push_view(info.vhv);
    push_view(info.vha);
    return 3;
}
#endif

static void FillJoinInfo (HashInfo_p info, V9View left, V9View right) {
    int g, r, gleft, nleft, nright, nused = 0, *hmap, *lmap, *jmap;
    V9View vjm, vhm, vlm;
    
    nleft = Head(left).count;
    vjm = NewPosView(nleft);
    jmap = vjm->col[0].p;

    InitHashInfo(info, left, NewPosView(nleft), HashVector(nleft),
                                                            HashValues(left));

    for (r = 0; r < nleft; ++r) {
        g = HashFind(left, r, info->hashes[r], info);
        if (g < 0)
            g = hiFill(info) - 1;
        jmap[r] = g;
    }

    /* TODO: reclaim unused entries at end of map */
    Head(info->vim).count = Head(info->imap).count = gleft = hiFill(info);
    assert(nleft == Head(info->hashes).count);
    nright = Head(right).count;
    
    vhm = NewPosView(gleft);
    hmap = vhm->col[0].p;
    vlm = NewPosView(nright);
    lmap = vlm->col[0].p;

    for (r = 0; r < nright; ++r) {
        g = HashFind(right, r, RowHash(right, r), info);
        if (g >= 0) {
            lmap[r] = hmap[g] - 1;
            hmap[g] = r + 1;
            ++nused;
        }
    }

    /* we're reusing veccol, but it might not be large enough to start with */
    /* TODO: reclaim unused entries at end of hvec */
    if (Head(info->hvec).count < nused) {
        V9_Release(info->vhv);
        info->vhv = V9_AddRef(NewPosView(nused));
        info->hvec = info->vhv->col[0].p;
    } else 
        Head(info->vhv).count = Head(info->hvec).count = nused;

    /* reorder output to match results from FillHashInfo and FillGroupInfo */
    info->vha = info->vhv;
    info->hashes = info->hvec;
    info->vhv = info->vim;
    info->hvec = info->imap;
    info->vim = vjm;
    info->imap = jmap;
    
    ChaseLinks(info, nused, hmap, lmap, gleft);
    
    /*  As with FillGroupInfo, this is most likely not quite optimal yet.
        All zero-length groups in smap (info->map, now info->hvec) could be 
        coalesced into one, and joinmap indices into it adjusted down a bit.
        Would reduce the size of smap when there are lots of failed matches.
        Also: FillJoinInfo needs quite a lot of temp vector space right now.
     */
}

#if 0
static int h_joininfo (vqEnv env) {
    HashInfo info;
    V9View v = check_view(env, 1), w = check_view(env, 2);
    FillJoinInfo(&info, v, w);
    push_view(info.vim);
    push_view(info.vhv);
    push_view(info.vha);
    return 3;
}
#endif

static int ViewCompat (V9View v1, V9View v2) {
    UNUSED(v1);
    UNUSED(v2);
    /* see MetaIsCompatible in v4 */
    return 1; /* FIXME: implement ViewCompat */
}

static V9View BufferAsPosView (Buffer* bp) {
    int cnt;
    V9View v = NewPosView(BufferCount(bp) / sizeof(int));
    char *data = v->col[0].p;
    char *ptr = 0;
    for (; NextFromBuffer(bp, &ptr, &cnt); data += cnt)
        memcpy(data, ptr, cnt);
    return v;
}

V9View V9_IntersectMap (V9View keys, V9View view) {
    int r;
    HashInfo info;
    struct Buffer buffer;
    
    if (!ViewCompat(keys, view)) 
        return 0;
        
    FillHashInfo(&info, view);
    InitBuffer(&buffer);
    
    /* these ints are added in increasing order, could have used a bitmap */
    for (r = 0; r < Head(keys).count; ++r)
        if (HashFind(keys, r, RowHash(keys, r), &info) >= 0)
            ADD_INT_TO_BUF(buffer, r);

    return BufferAsPosView(&buffer);
}

V9View V9_ExceptMap (V9View keys, V9View view) {
    int r;
    HashInfo info;
    struct Buffer buffer;
    
    if (!ViewCompat(keys, view)) 
        return 0;
        
    FillHashInfo(&info, view);
    InitBuffer(&buffer);
    
    /* these ints are added in increasing order, could have used a bitmap */
    for (r = 0; r < Head(keys).count; ++r)
        if (HashFind(keys, r, RowHash(keys, r), &info) < 0)
            ADD_INT_TO_BUF(buffer, r);

    return BufferAsPosView(&buffer);
}

/* RevIntersectMap returns RHS indices, instead of IntersectMap's LHS */
V9View V9_RevIntersectMap (V9View keys, V9View view) {
    int r;
    HashInfo info;
    struct Buffer buffer;
    
    FillHashInfo(&info, view);
    InitBuffer(&buffer);
    
    for (r = 0; r < Head(keys).count; ++r) {
        int f = HashFind(keys, r, RowHash(keys, r), &info);
        if (f >= 0)
            ADD_INT_TO_BUF(buffer, f);
    }
    
    return BufferAsPosView(&buffer);
}

V9View V9_UniqMap (V9View view) {
    HashInfo info;
    FillHashInfo(&info, view);
    return info.vim;
}
