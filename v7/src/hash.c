/*  Implementation of hashing functions.
    $Id$
    This file is part of Vlerq, see src/vlerq.h for full copyright notice. */

static vqView (HashValues) (vqView view); /* forward */
    
typedef struct HashInfo *HashInfo_p;

typedef struct HashInfo {
    vqView  view;          /* input view */
    vqView  vim, vhv, vha; /* views for the three following columns */
    int    *imap;          /* map of unique rows */
    int    *hvec;          /* hash probe vector (with prime & fill at end) */
    int    *hashes;        /* hash values, one per view row */
} HashInfo;

#define hiPrime(hi) (hi)->hvec[vSize((vqColumn) (hi)->vhv)-2]
#define hiFill(hi) (hi)->hvec[vSize((vqColumn) (hi)->vhv)-1]

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

static vqView HashCol (vqEnv env, vqSlot column) {
    int i, count, *data;
    vqSlot item;
    vqView v;

    count = vSize(column.c);
    v = NewPosView(env, count);
    data = v->col[0].p;

    for (i = 0; i < count; ++i) {
        item = column;
        switch (getcell(i, &item)) {
            case VQ_int:
            case VQ_pos:
                data[i] = item.i;
                break;
            case VQ_long:
                data[i] = item.i ^ item.x.y.i;
                break;
            case VQ_float:
                data[i] = item.i;
                break;
            case VQ_double:
                data[i] = item.i ^ item.x.y.i;
                break;
            case VQ_string:
                data[i] = StringHash(item.s, strlen(item.s));
                break;
            case VQ_bytes:
                data[i] = StringHash(item.s, item.x.y.i);
                break;
            case VQ_view: {
                vqView hashes = vq_incref(HashValues(item.v));
                const int *hvec = hashes->col[0].p;
                int j, hcount = vSize(hashes), hval = 0;
                for (j = 0; j < hcount; ++j)
                    hval ^= hvec[j];        
                data[i] = hval ^ hcount;
                vq_decref(hashes);
                break;
            }
            default: assert(0);
        }
    }

    return v;
}

#if 0
vqType HashColCmd_SO (vqSlot args[]) {
    vqType type = CharAsItemType(args[0].s[0]);
    vqSlot column = CoerceColumn(type, args[1].o);
    if (column.p == 0)
        return VQ_nil;        
    *args = HashCol(column);
    return IT_column;
}
#endif

static int RowHash (vqView view, int row) {
    int c, hash = 0;

    for (c = 0; c < vwCols(view); ++c) {
        vqSlot item = view->col[c];
        switch (getcell(row, &item)) {

            case VQ_int:
            case VQ_pos:
                hash ^= item.i;
                break;

            case VQ_string:
                hash ^= StringHash(item.s, strlen(item.s));
                break;

            case VQ_bytes:
                hash ^= StringHash(item.s, item.x.y.i);
                break;

            default: {
                /* TODO: ugly, using temp col with single int to map to view */
                vqView tview = NewPosView(vwEnv(view), 1), vhv;
                vq_setInt(tview, 0, 0, row);
                vhv = vq_incref(HashValues(RowMapVop(view, tview)));
                hash = vq_getInt(vhv, 0, 0, -1);
                vq_decref(vhv);
                break;
            }
        }
    }
    
    return hash;
}

static void XorWithIntCol (int *src, int *dest) {
    int i, count = vSize((vqColumn) src);

    for (i = 0; i < count; ++i)
        dest[i] ^= src[i];
}

static vqView HashValues (vqView view) {
    int c, *ivec, nr = vSize(view), nc = vwCols(view);
    vqView t;

    if (nc == 0 || nr == 0)
        return NewPosView(vwEnv(view), nr);
        
    t = HashCol(vwEnv(view), view->col[0]);
    ivec = t->col[0].p;
    for (c = 1; c < nc; ++c) {
        vqView u = vq_incref(HashCol(vwEnv(view), view->col[c]));
        int *auxcol = u->col[0].p;
        /* TODO: get rid of the separate xor step by xoring in HashCol */
        XorWithIntCol(auxcol, ivec);
        vq_decref(u);
    }
    return t;
}

static int HashFind (vqView keyview, int keyrow, int keyhash, HashInfo_p data) {
    int probe, datarow, mask, step, prime;

    mask = vSize(data->vhv) - 3;
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
                RowEqual(keyview, keyrow, data->view, datarow))
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

static vqView HashVector (vqEnv env, int rows) {
    static char slack [] = {
        0, 0, 3, 3, 3, 5, 3, 3, 29, 17, 9, 5, 83, 27, 43, 3,
        45, 9, 39, 39, 9, 5, 3, 33, 27, 9, 71, 39, 9, 5, 83, 0
    };

    vqView v;
    int bits = Log2bits((4 * rows) / 3), *iptr, n;
    if (bits < 2)
        bits = 2;
    n = 1 << bits;
    v = NewPosView(env, n+2);
    iptr = v->col[0].p;
    iptr[n] = n + slack[Log2bits(n-1)]; /* prime */
    return v;
}

static void InitHashInfo (HashInfo_p info, vqView view, vqView vim, vqView vhv, vqView vha) {
    info->view = view;
    info->vim = vq_incref(vim);
    info->imap = vim->col[0].p;
    info->vhv = vq_incref(vhv);
    info->hvec = vhv->col[0].p;
    info->vha = vq_incref(vha);
    info->hashes = vha->col[0].p;
    
    /* FIXME: cleanup! */
}

#if 0 /* not used right now */
static int StringHashFind (const char *key, vqColumn hseq, vqSlot values) {
    int probe, datarow, mask, step, keyhash;
    const int *hvec = (const int*) hseq;
    int prime = vExtra(hseq);
    vqSlot cell;
    
    keyhash = StringHash(key, -1);
    mask = vSize(hseq) - 1;
    probe = ~keyhash & mask;

    step = (keyhash ^ (keyhash >> 3)) & mask;
    if (step == 0)
        step = mask;

    for (;;) {
        probe = (probe + step) & mask;
        datarow = hvec[probe] - 1;
        if (datarow < 0)
            break;

        /* These string hashes are much simpler than the standard HashFind:
             no hashes vector, no indirect map, compute all hashes on-the-fly */
                
        cell = values;
        getcell(datarow, &cell);
        if (strcmp(key, cell.s) == 0)
            return datarow;

        step <<= 1;
        if (step > mask)
            step ^= prime;
    }

    return ~probe;
}

static int StringLookup (const char *key, vqSlot values) {
    int h, r, rows, *hptr;
    vqColumn hvec;
    HashInfo info;
    vqSlot cell;
    
    /* adjust vExtra, this assumes values is a string column */
    
    if (vExtra(values.p) == 0) {
        rows = vSize(values.p);
        hvec = HashVector(rows);
        hptr = (int*) hvec;
        
        vExtra(hvec) = HashPrime(rows);
        
        for (r = 0; r < rows; ++r) {
            cell = values;
            getcell(r, &cell);
            h = StringHashFind(cell.s, hvec, values);
            if (h < 0) /* silently ignore duplicates */
                hptr[~h] = r + 1;
        }
        
        vExtra(values.p) = (int) vq_increfx(hvec); /* FIXME: 64b! */
    }
    
    h = StringHashFind(key, (vqColumn) vExtra(values.p), values);
    return h >= 0 ? h : -1;
}
#endif

static void FillHashInfo (HashInfo_p info, vqView view) {
    int r, rows = vSize(view);

    InitHashInfo(info, view, NewPosView(vwEnv(view), rows),
                                HashVector(vwEnv(view), rows),
                                HashValues(view));

    for (r = 0; r < rows; ++r)
        HashFind(view, r, info->hashes[r], info);

    /* TODO: reclaim unused entries at end of map */
    vSize(info->vim) = vSize((vqColumn) info->imap) = hiFill(info);
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

static void FillGroupInfo (HashInfo_p info, vqView view) {
    int g, r, rows, *hmap, *lmap;
    vqView vhm, vlm;
    vqEnv env = vwEnv(view);
    
    rows = vSize(view);
    vhm = NewPosView(env, rows);
    hmap = vhm->col[0].p;
    vlm = NewPosView(env, rows);
    lmap = vlm->col[0].p;

    InitHashInfo(info, view, NewPosView(env, rows),
                                HashVector(env, rows),
                                HashValues(view));

    for (r = 0; r < rows; ++r) {
        g = HashFind(view, r, info->hashes[r], info);
        if (g < 0)
            g = hiFill(info) - 1;
        lmap[r] = hmap[g] - 1;
        hmap[g] = r + 1;
    }
    
    /* TODO: reclaim unused entries at end of map and hvec */
    vSize(info->vim) = vSize((vqColumn) info->imap) = hiFill(info);
    
    ChaseLinks(info, rows, hmap, lmap, hiFill(info));

    vSize(info->vhv) = vSize((vqColumn) info->hvec) =
                                        vSize((vqColumn) info->imap);
    
    /* TODO: could release headmap and linkmap but that's a no-op here */
    
    /*  There's probably an opportunity to reduce space usage further,
        since the grouping map points to the starting row of each group:
            map[i] == gmap[smap[i]]
        Perhaps "map" (which starts out with #rows entries) can be re-used
        to append the gmap entries (if we can reduce it by #groups items).
        Or just release map[x] for grouping views, and use gmap[smap[x]].
    */
}

static int h_groupinfo (vqEnv env) {
    HashInfo info;
    vqView v = check_view(env, 1);
    FillGroupInfo(&info, v);
    push_view(info.vim);
    push_view(info.vhv);
    push_view(info.vha);
    return 3;
}

static void FillJoinInfo (HashInfo_p info, vqView left, vqView right) {
    int g, r, gleft, nleft, nright, nused = 0, *hmap, *lmap, *jmap;
    vqView vjm, vhm, vlm;
    vqEnv env = vwEnv(left);
    
    nleft = vSize(left);
    vjm = NewPosView(env, nleft);
    jmap = vjm->col[0].p;

    InitHashInfo(info, left, NewPosView(env, nleft),
                                HashVector(env, nleft),
                                HashValues(left));

    for (r = 0; r < nleft; ++r) {
        g = HashFind(left, r, info->hashes[r], info);
        if (g < 0)
            g = hiFill(info) - 1;
        jmap[r] = g;
    }

    /* TODO: reclaim unused entries at end of map */
    vSize(info->vim) = vSize((vqColumn) info->imap) = gleft = hiFill(info);
    assert(nleft == vSize((vqColumn) info->hashes));
    nright = vSize(right);
    
    vhm = NewPosView(env, gleft);
    hmap = vhm->col[0].p;
    vlm = NewPosView(env, nright);
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
    if (vSize((vqColumn) info->hvec) < nused) {
        vq_decref(info->vhv);
        info->vhv = vq_incref(NewPosView(env, nused));
        info->hvec = info->vhv->col[0].p;
    } else 
        vSize(info->vhv) = vSize((vqColumn) info->hvec) = nused;

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

static int h_joininfo (vqEnv env) {
    HashInfo info;
    vqView v = check_view(env, 1), w = check_view(env, 2);
    FillJoinInfo(&info, v, w);
    push_view(info.vim);
    push_view(info.vhv);
    push_view(info.vha);
    return 3;
}

static int ViewCompat (vqView v1, vqView v2) {
    VQ_UNUSED(v1);
    VQ_UNUSED(v2);
    /* see MetaIsCompatible in v4 */
    return 1; /* FIXME: implement ViewCompat */
}

static vqView IntersectMap (vqView keys, vqView view) {
    int r;
    HashInfo info;
    struct Buffer buffer;
    
    if (!ViewCompat(keys, view)) 
        return 0;
        
    FillHashInfo(&info, view);
    InitBuffer(&buffer);
    
    /* these ints are added in increasing order, could have used a bitmap */
    for (r = 0; r < vSize(keys); ++r)
        if (HashFind(keys, r, RowHash(keys, r), &info) >= 0)
            ADD_INT_TO_BUF(buffer, r);

    return BufferAsPosView(vwEnv(keys), &buffer);
}

static vqView ExceptMap (vqView keys, vqView view) {
    int r;
    HashInfo info;
    struct Buffer buffer;
    
    if (!ViewCompat(keys, view)) 
        return 0;
        
    FillHashInfo(&info, view);
    InitBuffer(&buffer);
    
    /* these ints are added in increasing order, could have used a bitmap */
    for (r = 0; r < vSize(keys); ++r)
        if (HashFind(keys, r, RowHash(keys, r), &info) < 0)
            ADD_INT_TO_BUF(buffer, r);

    return BufferAsPosView(vwEnv(keys), &buffer);
}

/* ReverseIntersectMap returns RHS indices, instead of IntersectMap's LHS */
static vqView ReverseIntersectMap (vqView keys, vqView view) {
    int r;
    HashInfo info;
    struct Buffer buffer;
    
    FillHashInfo(&info, view);
    InitBuffer(&buffer);
    
    for (r = 0; r < vSize(keys); ++r) {
        int f = HashFind(keys, r, RowHash(keys, r), &info);
        if (f >= 0)
            ADD_INT_TO_BUF(buffer, f);
    }
    
    return BufferAsPosView(vwEnv(keys), &buffer);
}

static int h_revisect (vqEnv env) {
    vqView v = check_view(env, 1), w = check_view(env, 2);
    return push_view(ReverseIntersectMap(v, w));
}

static vqView UniqMap (vqView view) {
    HashInfo info;
    FillHashInfo(&info, view);
    return info.vim;
}

#if 0
static int HashDoFind (vqView view, int row, vqView w, vqColumn a, vqColumn b, vqColumn c) {
    HashInfo info;
    /* TODO: avoid Log2bits call in InitHashInfo, since done on each find */
    InitHashInfo(&info, w, a, b, c);
    return HashFind(view, row, RowHash(view, row), &info);
}
static vqColumn GetHashInfo (vqView left, vqView right, int type) {
    HashInfo info;
    vqColumn seqvec[3];

    switch (type) {
        case 0:     FillHashInfo(&info, left); break;
        case 1:     FillGroupInfo(&info, left); break;
        default:    FillJoinInfo(&info, left, right); break;
    }
        
    seqvec[0] = info.mapcol;
    seqvec[1] = info.veccol;
    seqvec[2] = info.hashcol;

    return 0; /* FIXME: NewSeqVec(IT_column, seqvec, 3); */
}
static vqView IjoinView (vqView left, vqView right) {
    vqView view = JoinView(left, right, "?");
    return UngroupView(view, vwCols(view)-1);
}
static vqDispatch ST_HashMap = { "hashmap", 0, 02 };
static vqColumn HashMapNew (void) {
#if 0
    vqColumn result;
    
    result = NewSequence(0, &ST_HashMap, 0);
    /* data[0] is the key + value + hash vector */
    /* data[1] is the allocated count */
    result->data[0].p = 0;
    result->data[1].i = 0;
    
    return result;
#endif
    return 0;
}
/* FIXME: dumb linear scan instead of hashing for now, for small tests only */
static int HashMapLocate(vqColumn hmap, int key, int *pos) {
    const int *data = (const int*) hmap;
    for (*pos = 0; *pos < vSize(hmap); ++*pos)
        if (key == data[*pos])
            return 1;
    return 0;
}
static int HashMapAdd (vqColumn hmap, int key, int value) {
    VQ_UNUSED(hmap);
    VQ_UNUSED(key);
    VQ_UNUSED(value);
#if 0
    int pos, *data = (int*) hmap;
    int allocnt = hmap->data[1].i;

    if (HashMapLocate(hmap, key, &pos)) {
        data[pos+allocnt] = value;
        return 0;
    }

    assert(pos == vSize(hmap));
    if (pos >= allocnt) {
        int newlen = (allocnt / 2) * 3 + 10;
        hmap = data = realloc(data, newlen * 2 * sizeof(int));
        memmove(data+newlen, data+allocnt, allocnt * sizeof(int));
        allocnt = hmap->data[1].i = newlen;
    }
    
    data[pos] = key;
    data[pos+allocnt] = value;
    ++vSize(hmap);
    return allocnt;
#endif
    return -1;
}
static int HashMapLookup (vqColumn hmap, int key, int defval) {
    VQ_UNUSED(hmap);
    VQ_UNUSED(key);
    VQ_UNUSED(defval);
#if 0
    int pos;

    if (HashMapLocate(hmap, key, &pos)) {
        int allocnt = hmap->data[1].i;
        const int *data = (const int*) hmap;
        return data[pos+allocnt];
    }
    
    return defval;
#endif
    return -1;
}
int HashMapRemove (vqColumn hmap, int key) {
    int pos;

    if (HashMapLocate(hmap, key, &pos)) {
        int last = --vSize(hmap);
        if (pos < last) {
            int allocnt = hmap->data[1].i, *data = hmap->data[0].p;
            data[pos] = data[last];
            data[pos+allocnt] = data[last+allocnt];
            return pos;
        }
    }
    
    return -1;
}
#endif

static int h_exceptmap (vqEnv env) {
    return push_view(ExceptMap(check_view(env, 1), check_view(env, 2)));
}

static int h_isectmap (vqEnv env) {
    return push_view(IntersectMap(check_view(env, 1), check_view(env, 2)));
}

static int h_uniquemap (vqEnv env) {
    return push_view(UniqMap(check_view(env, 1)));
}

static const struct luaL_reg vqlib_hash[] = {
    {"exceptmap", h_exceptmap},
    {"groupinfo", h_groupinfo},
    {"joininfo", h_joininfo},
    {"isectmap", h_isectmap},
    {"revisect", h_revisect},
    {"uniquemap", h_uniquemap},
    {0, 0},
};
