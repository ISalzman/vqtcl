/*  Implementation of hashing functions.
    $Id$
    This file is part of Vlerq, see src/vlerq.h for full copyright notice. */

static vqVec (HashValues) (vqView view); /* forward */
    
typedef struct HashInfo *HashInfo_p;

typedef struct HashInfo {
    vqView  view;       /* input view */
    int     prime;      /* prime used for hashing */
    int     fill;       /* used to fill map */
    int    *map;        /* map of unique rows */
    vqVec   mapcol;     /* owner of map */
    int    *hvec;       /* hash probe vector */
    vqVec   veccol;     /* owner of hvec */
    int    *hashes;     /* hash values, one per view row */
    vqVec   hashcol;    /* owner of hashes */
} HashInfo;

static int StringHash (const char *s, int n) {
    /* similar to Python's stringobject.c */
    int i, h = (*s * 0xFF) << 7;
    if (n < 0)
        n = strlen(s);
    for (i = 0; i < n; ++i)
        h = (1000003 * h) ^ s[i];
    return h ^ i;
}

static vqVec HashCol (vqCell column) {
    int i, count, *data;
    vqVec seq;
    vqCell item;

    count = vSize(column.p);
    seq = NewIntVec(count, &data);

    for (i = 0; i < count; ++i) {
        item = column;
        switch (getcell(i, &item)) {
            case VQ_int:
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
                data[i] = StringHash(item.s, -1);
                break;
            case VQ_bytes:
                data[i] = StringHash(item.s, item.x.y.i);
                break;
            case VQ_view: {
                vqVec hashes = HashValues(item.v);
                const int *hvec = (const int*) hashes;
                int j, hcount = vSize(hashes), hval = 0;
                            
                for (j = 0; j < hcount; ++j)
                    hval ^= hvec[j];
                /* TODO: release hashes right now */
        
                data[i] = hval ^ hcount;
                break;
            }
            default: assert(0);
        }
    }

    return seq;
}

#if 0
vqType HashColCmd_SO (vqCell args[]) {
    vqType type = CharAsItemType(args[0].s[0]);
    vqCell column = CoerceColumn(type, args[1].o);
    if (column.p == 0)
        return VQ_nil;        
    *args = HashCol(column);
    return IT_column;
}
#endif

static int RowHash (vqView view, int row) {
    int c, hash = 0;
    vqCell item;

    for (c = 0; c < vwCols(view); ++c) {
        item = vwCol(view, c);
        switch (getcell(row, &item)) {

            case VQ_int:
                hash ^= item.i;
                break;

            case VQ_string:
                hash ^= StringHash(item.s, -1);
                break;

            case VQ_bytes:
                hash ^= StringHash(item.s, item.x.y.i);
                break;

            default: {
#if 0
                vqView rview = StepView(view, 1, row, 1, 1);
#else
                vqView rview = view; /* FIXME: should only hash a single row */
#endif
                vqVec rcol = vq_incref(HashValues(rview));
                hash = *(const int*) rcol;
                vq_decref(rcol);
                break;
            }
        }
    }

    return hash;
}

static void XorWithIntCol (vqVec src, vqVec dest) {
    const int *srcdata = (const int*) src;
    int i, count = vSize(src), *destdata = (int*) dest;

    for (i = 0; i < count; ++i)
        destdata[i] ^= srcdata[i];
}

static vqVec HashValues (vqView view) {
    int c;
    vqVec result;

    if (vwCols(view) == 0 || vwRows(view) == 0)
        return NewIntVec(vwRows(view), 0);
        
    result = HashCol(vwCol(view, 0));
    for (c = 1; c < vwCols(view); ++c) {
        vqVec auxcol = vq_incref(HashCol(vwCol(view, c)));
        /* TODO: get rid of the separate xor step by xoring in HashCol */
        XorWithIntCol(auxcol, result);
        vq_decref(auxcol);
    }
    return result;
}

static int HashFind (vqView keyview, int keyrow, int keyhash, HashInfo_p data) {
    int probe, datarow, mask, step;

    mask = vSize(data->veccol) - 1;
    probe = ~keyhash & mask;

    step = (keyhash ^ (keyhash >> 3)) & mask;
    if (step == 0)
        step = mask;

    for (;;) {
        probe = (probe + step) & mask;
        if (data->hvec[probe] == 0)
            break;

        datarow = data->map[data->hvec[probe]-1];
        if (keyhash == data->hashes[datarow] &&
                RowEqual(keyview, keyrow, data->view, datarow))
            return data->hvec[probe] - 1;

        step <<= 1;
        if (step > mask)
            step ^= data->prime;
    }

    if (keyview == data->view) {
        data->hvec[probe] = data->fill + 1;
        data->map[data->fill++] = keyrow;
    }

    return -1;
}

static int StringHashFind (const char *key, vqVec hseq, vqCell values) {
    int probe, datarow, mask, step, keyhash;
    const int *hvec = (const int*) hseq;
    int prime = hseq->data[2].i;
    
    keyhash = StringHash(key, -1);
    mask = hseq->count - 1;
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
                
        if (strcmp(key, GetColItem(datarow, values, VQ_string).s) == 0)
            return datarow;

        step <<= 1;
        if (step > mask)
            step ^= prime;
    }

    return ~probe;
}

static int Log2bits (int n) {
    int bits = 0;
    while ((1 << bits) < n)
        ++bits;
    return bits;
}

static vqVec HashVector (int rows) {
    int bits = Log2bits((4 * rows) / 3);
    if (bits < 2)
        bits = 2;
    return NewIntVec(1 << bits, 0);
}

static void InitHashInfo (HashInfo_p info, vqView view, vqVec hmap, vqVec hvec, vqVec hashes) {
    int size = vSize(hvec.p);

    static char slack [] = {
        0, 0, 3, 3, 3, 5, 3, 3, 29, 17, 9, 5, 83, 27, 43, 3,
        45, 9, 39, 39, 9, 5, 3, 33, 27, 9, 71, 39, 9, 5, 83, 0
    };

    info->view = view;
    info->prime = size + slack[Log2bits(size-1)];
    info->fill = 0;
    
    info->mapcol = hmap;
    info->map = (int*) hmap;

    info->veccol = hvec;
    info->hvec = (int*) hvec;

    info->hashcol = hashes;
    info->hashes = (int*) hashes;
}

int StringLookup (const char *key, vqCell values) {
    int h, r, rows, *hptr;
    const char *string;
    vqVec hvec;
    HashInfo info;
    
    /* adjust data[2], this assumes values is a string column */
    
    if (values.p->data[2].p == 0) {
        rows = vSize(values.p);
        hvec = HashVector(rows);
        hptr = (int*) hvec;
        
        /* use InitHashInfo to get at the prime number, bit of a hack */
        InitHashInfo(&info, 0, hvec, hvec, hvec);
        hvec.p->data[2].i = info.prime;
        
        for (r = 0; r < rows; ++r) {
            string = GetColItem(r, values, VQ_string).s;
            h = StringHashFind(string, hvec, values);
            if (h < 0) /* silently ignore duplicates */
                hptr[~h] = r + 1;
        }
        
        values.p->data[2].p = vq_incref(hvec);
    }
    
    h = StringHashFind(key, (vqVec) values.p->data[2].p, values);
    return h >= 0 ? h : -1;
}

static void FillHashInfo (HashInfo_p info, vqView view) {
    int r, rows;
    vqVec mapcol;
    
    rows = vwRows(view);
    mapcol = NewIntVec(rows, 0); /* worst-case, dunno #groups */

    InitHashInfo(info, view, mapcol, HashVector(rows), HashValues(view));

    for (r = 0; r < rows; ++r)
        HashFind(view, r, info->hashes[r], info);

    /* TODO: reclaim unused entries at end of map */
    vSize(mapcol.p) = info->fill;
}

static void ChaseLinks (HashInfo_p info, int count, const int *hmap, const int *lmap) {
    int groups = info->fill, *smap = info->hvec, *gmap = info->hashes;
    
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

void FillGroupInfo (HashInfo_p info, vqView view) {
    int g, r, rows, *hmap, *lmap;
    vqVec mapcol, headmap, linkmap;
    
    rows = vwRows(view);
    mapcol = NewIntVec(rows, 0); /* worst-case, dunno #groups */
    headmap = NewIntVec(rows, &hmap); /* same: don't know #groups */
    linkmap = NewIntVec(rows, &lmap);

    InitHashInfo(info, view, mapcol, HashVector(rows), HashValues(view));

    for (r = 0; r < rows; ++r) {
        g = HashFind(view, r, info->hashes[r], info);
        if (g < 0)
            g = info->fill - 1;
        lmap[r] = hmap[g] - 1;
        hmap[g] = r + 1;
    }
    
    /* TODO: reclaim unused entries at end of map and hvec */
    vSize(info->mapcol.p) = vSize(info->veccol.p) = info->fill;
    
    ChaseLinks(info, rows, hmap, lmap);

    /* TODO: could release headmap and linkmap but that's a no-op here */
    
    /*  There's probably an opportunity to reduce space usage further,
        since the grouping map points to the starting row of each group:
            map[i] == gmap[smap[i]]
        Perhaps "map" (which starts out with #rows entries) can be re-used
        to append the gmap entries (if we can reduce it by #groups items).
        Or just release map[x] for grouping views, and use gmap[smap[x]].
    */
}

vqView GroupCol (vqView view, vqCell cols, const char *name) {
    vqView vkey, vres, gview;
    HashInfo info;
    
    vkey = ColMapView(view, cols);
    vres = ColMapView(view, OmitColumn(cols, vwCols(view)));

    FillGroupInfo(&info, vkey);
    gview = GroupedView(vres, info.veccol, info.hashcol, name);
    return PairView(RemapSubview(vkey, info.mapcol, 0, -1), gview);
}

static void FillJoinInfo (HashInfo_p info, vqView left, vqView right) {
    int g, r, gleft, nleft, nright, nused = 0, *hmap, *lmap, *jmap;
    vqVec mapcol, headmap, linkmap, joincol;
    
    nleft = vwRows(left);
    mapcol = NewIntVec(nleft, 0); /* worst-case dunno #groups */
    joincol = NewIntVec(nleft, &jmap);
    
    InitHashInfo(info, left, mapcol, HashVector(nleft), HashValues(left));

    for (r = 0; r < nleft; ++r) {
        g = HashFind(left, r, info->hashes[r], info);
        if (g < 0)
            g = info->fill - 1;
        jmap[r] = g;
    }

    /* TODO: reclaim unused entries at end of map */
    vSize(mapcol) = info->fill;

    gleft = vSize(info->mapcol);
    nleft = vSize(info->hashcol);
    nright = vwRows(right);
    
    headmap = NewIntVec(gleft, &hmap); /* don't know #groups */
    linkmap = NewIntVec(nright, &lmap);

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
    if (vSize(info->veccol) < nused)
        info->veccol = NewIntVec(nused, &info->hvec);
    else 
        vSize(info->veccol) = nused;

    /* reorder output to match results from FillHashInfo and FillGroupInfo */
    info->hashcol = info->veccol;
    info->hashes = info->hvec;
    info->veccol = info->mapcol;
    info->hvec = info->map;
    info->mapcol = joincol;
    info->map = jmap;
    
    ChaseLinks(info, nused, hmap, lmap);
    
    /*  As with FillGroupInfo, this is most likely not quite optimal yet.
        All zero-length groups in smap (info->map, now info->hvec) could be 
        coalesced into one, and joinmap indices into it adjusted down a bit.
        Would reduce the size of smap when there are lots of failed matches.
        Also: FillJoinInfo needs quite a lot of temp vector space right now.
     */
}

vqVec IntersectMap (vqView keys, vqView view) {
    int r, rows;
    HashInfo info;
    struct Buffer buffer;
    
    if (!ViewCompat(keys, view)) 
        return 0;
        
    FillHashInfo(&info, view);
    InitBuffer(&buffer);
    rows = vwRows(keys);
    
    /* these ints are added in increasing order, could have used a bitmap */
    for (r = 0; r < rows; ++r)
        if (HashFind(keys, r, RowHash(keys, r), &info) >= 0)
            ADD_INT_TO_BUF(buffer, r);

    return BufferAsIntVec(&buffer);
}

/* ReverseIntersectMap returns RHS indices, instead of IntersectMap's LHS */
static vqVec ReverseIntersectMap (vqView keys, vqView view) {
    int r, rows;
    HashInfo info;
    struct Buffer buffer;
    
    FillHashInfo(&info, view);
    InitBuffer(&buffer);
    rows = vwRows(keys);
    
    for (r = 0; r < rows; ++r) {
        int f = HashFind(keys, r, RowHash(keys, r), &info);
        if (f >= 0)
            ADD_INT_TO_BUF(buffer, f);
    }
    
    return BufferAsIntVec(&buffer);
}

vqView JoinView (vqView left, vqView right, const char *name) {
    vqView lmeta, rmeta, lkey, rkey, rres, gview;
    vqCell lmap, rmap;
    HashInfo info;
    
    lmeta = vwMeta(left);
    rmeta = vwMeta(right);
    
    lmap = IntersectMap(lmeta, rmeta);
    /* TODO: optimize, don't create the hash info twice */
    rmap = ReverseIntersectMap(lmeta, rmeta);

    lkey = ColMapView(left, lmap);
    rkey = ColMapView(right, rmap);
    rres = ColMapView(right, OmitColumn(rmap, vwCols(right)));

    FillJoinInfo(&info, lkey, rkey);
    gview = GroupedView(rres, info.veccol, info.hashcol, name);
    return PairView(left, RemapSubview(gview, info.mapcol, 0, -1));
}

vqCell UniqMap (vqView view) {
    HashInfo info;
    FillHashInfo(&info, view);
    return info.mapcol;
}

int HashDoFind (vqView view, int row, vqView w, vqVec a, vqVec b, vqVec c) {
    HashInfo info;
    /* TODO: avoid Log2bits call in InitHashInfo, since done on each find */
    InitHashInfo(&info, w, a, b, c);
    return HashFind(view, row, RowHash(view, row), &info);
}

vqVec GetHashInfo (vqView left, vqView right, int type) {
    HashInfo info;
    vqVec seqvec[3];

    switch (type) {
        case 0:     FillHashInfo(&info, left); break;
        case 1:     FillGroupInfo(&info, left); break;
        default:    FillJoinInfo(&info, left, right); break;
    }
        
    seqvec[0] = info.mapcol;
    seqvec[1] = info.veccol;
    seqvec[2] = info.hashcol;

    return NewSeqVec(IT_column, seqvec, 3);
}

vqView IjoinView (vqView left, vqView right) {
    vqView view = JoinView(left, right, "?");
    return UngroupView(view, vwCols(view)-1);
}

static vqDispatch ST_HashMap = { "hashmap", 0, 02 };

vqVec HashMapNew (void) {
    vqVec result;
    
    result = NewSequence(0, &ST_HashMap, 0);
    /* data[0] is the key + value + hash vector */
    /* data[1] is the allocated count */
    result->data[0].p = 0;
    result->data[1].i = 0;
    
    return result;
}

/* FIXME: dumb linear scan instead of hashing for now, for small tests only */

static int HashMapLocate(vqVec hmap, int key, int *pos) {
    const int *data = (const int*) hmap;
    for (*pos = 0; *pos < hmap->count; ++*pos)
        if (key == data[*pos])
            return 1;
    return 0;
}

int HashMapAdd (vqVec hmap, int key, int value) {
    int pos, *data = (int*) hmap;
    int allocnt = hmap->data[1].i;

    if (HashMapLocate(hmap, key, &pos)) {
        data[pos+allocnt] = value;
        return 0;
    }

    assert(pos == hmap->count);
    if (pos >= allocnt) {
        int newlen = (allocnt / 2) * 3 + 10;
        hmap = data = realloc(data, newlen * 2 * sizeof(int));
        memmove(data+newlen, data+allocnt, allocnt * sizeof(int));
        allocnt = hmap->data[1].i = newlen;
    }
    
    data[pos] = key;
    data[pos+allocnt] = value;
    ++hmap->count;
    return allocnt;
}

int HashMapLookup (vqVec hmap, int key, int defval) {
    int pos;

    if (HashMapLocate(hmap, key, &pos)) {
        int allocnt = hmap->data[1].i;
        const int *data = (const int*) hmap;
        return data[pos+allocnt];
    }
    
    return defval;
}

#if 0 /* not yet */
int HashMapRemove (vqVec hmap, int key) {
    int pos;

    if (HashMapLocate(hmap, key, &pos)) {
        int last = --hmap->count;
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
