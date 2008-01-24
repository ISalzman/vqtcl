/*
 * hash.c - Implementation of hashing functions.
 */

#include <stdlib.h>
#include <string.h>

#include "intern.h"
#include "wrap_gen.h"

typedef struct HashInfo *HashInfo_p;

typedef struct HashInfo {
    View_p  view;       /* input view */
    int     prime;      /* prime used for hashing */
    int     fill;       /* used to fill map */
    int    *map;        /* map of unique rows */
    Column  mapcol;     /* owner of map */
    int    *hvec;       /* hash probe vector */
    Column  veccol;     /* owner of hvec */
    int    *hashes;     /* hash values, one per view row */
    Column  hashcol;    /* owner of hashes */
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

static Column HashCol (ItemTypes type, Column column) {
    int i, count, *data;
    Seq_p seq;
    Item item;

/* the following is not possible: the result may be xor-ed into!
    if (type == IT_int && column.seq->getter == PickIntGetter(32))
        return column;
*/

    count = column.seq->count;
    seq = NewIntVec(count, &data);

    switch (type) {
        
        case IT_int:
            for (i = 0; i < count; ++i)
                data[i] = GetColItem(i, column, IT_int).i;
            break;
                
        case IT_wide:
            for (i = 0; i < count; ++i) {
                item = GetColItem(i, column, IT_wide);
                data[i] = item.q[0] ^ item.q[1];
            }
            break;
                
        case IT_float:
            for (i = 0; i < count; ++i)
                data[i] = GetColItem(i, column, IT_float).i;
            break;
                
        case IT_double:
            for (i = 0; i < count; ++i) {
                item = GetColItem(i, column, IT_double);
                data[i] = item.q[0] ^ item.q[1];
            }
            break;
                
        case IT_string:
            for (i = 0; i < count; ++i) {
                item = GetColItem(i, column, IT_string);
                data[i] = StringHash(item.s, -1);
            }
            break;
                
        case IT_bytes:
            for (i = 0; i < count; ++i) {
                item = GetColItem(i, column, IT_bytes);
                data[i] = StringHash((const char*) item.u.ptr, item.u.len);
            }
            break;
                
        case IT_view:
            for (i = 0; i < count; ++i) {
                int j, hcount, hval = 0;
                const int *hvec;
                Column hashes;
                
                item = GetColItem(i, column, IT_view);
                hashes = HashValues(item.v);
                hvec = (const int*) hashes.seq->data[0].p;
                hcount = hashes.seq->count;
                
                for (j = 0; j < hcount; ++j)
                    hval ^= hvec[j];
                /* TODO: release hashes right now */
            
                data[i] = hval ^ hcount;
            }
            break;
                
        default: Assert(0);
    }

    return SeqAsCol(seq);
}

ItemTypes HashColCmd_SO (Item args[]) {
    ItemTypes type;
    Column column;
    
    type = CharAsItemType(args[0].s[0]);
    column = CoerceColumn(type, args[1].o);
    if (column.seq == NULL)
        return IT_unknown;
        
    args->c = HashCol(type, column);
    return IT_column;
}

int RowHash (View_p view, int row) {
    int c, hash = 0;
    Item item;

    for (c = 0; c < ViewWidth(view); ++c) {
        item.c = ViewCol(view, c);
        switch (GetItem(row, &item)) {

            case IT_int:
                hash ^= item.i;
                break;

            case IT_string:
                hash ^= StringHash(item.s, -1);
                break;

            case IT_bytes:
                hash ^= StringHash((const char*) item.u.ptr, item.u.len);
                break;

            default: {
                View_p rview = StepView(view, 1, row, 1, 1);
                Column rcol = HashValues(rview);
                hash = *(const int*) rcol.seq->data[0].p;
                break;
            }
        }
    }

    return hash;
}

static void XorWithIntCol (Column src, Column_p dest) {
    const int *srcdata = (const int*) src.seq->data[0].p;
    int i, count = src.seq->count, *destdata = (int*) dest->seq->data[0].p;

    for (i = 0; i < count; ++i)
        destdata[i] ^= srcdata[i];
}

Column HashValues (View_p view) {
    int c;
    Column result;

    if (ViewWidth(view) == 0 || ViewSize(view) == 0)
        return SeqAsCol(NewIntVec(ViewSize(view), NULL));
        
    result = HashCol(ViewColType(view, 0), ViewCol(view, 0));
    for (c = 1; c < ViewWidth(view); ++c) {
        Column auxcol = HashCol(ViewColType(view, c), ViewCol(view, c));
        /* TODO: get rid of the separate xor step by xoring in HashCol */
        XorWithIntCol(auxcol, &result);
    }
    return result;
}

static int HashFind (View_p keyview, int keyrow, int keyhash, HashInfo_p data) {
    int probe, datarow, mask, step;

    mask = data->veccol.seq->count - 1;
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

static int StringHashFind (const char *key, Seq_p hseq, Column values) {
    int probe, datarow, mask, step, keyhash;
    const int *hvec = (const int*) hseq->data[0].p;
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
                
        if (strcmp(key, GetColItem(datarow, values, IT_string).s) == 0)
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

static Column HashVector (int rows) {
    int bits = Log2bits((4 * rows) / 3);
    if (bits < 2)
        bits = 2;
    return SeqAsCol(NewIntVec(1 << bits, NULL));
}

static void InitHashInfo (HashInfo_p info, View_p view, Column hmap, Column hvec, Column hashes) {
    int size = hvec.seq->count;

    static char slack [] = {
        0, 0, 3, 3, 3, 5, 3, 3, 29, 17, 9, 5, 83, 27, 43, 3,
        45, 9, 39, 39, 9, 5, 3, 33, 27, 9, 71, 39, 9, 5, 83, 0
    };

    info->view = view;
    info->prime = size + slack[Log2bits(size-1)];
    info->fill = 0;
    
    info->mapcol = hmap;
    info->map = (int*) hmap.seq->data[0].p;

    info->veccol = hvec;
    info->hvec = (int*) hvec.seq->data[0].p;

    info->hashcol = hashes;
    info->hashes = (int*) hashes.seq->data[0].p;
}

int StringLookup (const char *key, Column values) {
    int h, r, rows, *hptr;
    const char *string;
    Column hvec;
    HashInfo info;
    
    /* adjust data[2], this assumes values is a string column */
    
    if (values.seq->data[2].p == NULL) {
        rows = values.seq->count;
        hvec = HashVector(rows);
        hptr = (int*) hvec.seq->data[0].p;
        
        /* use InitHashInfo to get at the prime number, bit of a hack */
        InitHashInfo(&info, NULL, hvec, hvec, hvec);
        hvec.seq->data[2].i = info.prime;
        
        for (r = 0; r < rows; ++r) {
            string = GetColItem(r, values, IT_string).s;
            h = StringHashFind(string, hvec.seq, values);
            if (h < 0) /* silently ignore duplicates */
                hptr[~h] = r + 1;
        }
        
        values.seq->data[2].p = IncRefCount(hvec.seq);
    }
    
    h = StringHashFind(key, (Seq_p) values.seq->data[2].p, values);
    return h >= 0 ? h : -1;
}

static void FillHashInfo (HashInfo_p info, View_p view) {
    int r, rows;
    Column mapcol;
    
    rows = ViewSize(view);
    mapcol = SeqAsCol(NewIntVec(rows, NULL)); /* worst-case, dunno #groups */

    InitHashInfo(info, view, mapcol, HashVector(rows), HashValues(view));

    for (r = 0; r < rows; ++r)
        HashFind(view, r, info->hashes[r], info);

    /* TODO: reclaim unused entries at end of map */
    mapcol.seq->count = info->fill;
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

void FillGroupInfo (HashInfo_p info, View_p view) {
    int g, r, rows, *hmap, *lmap;
    Column mapcol, headmap, linkmap;
    
    rows = ViewSize(view);
    mapcol = SeqAsCol(NewIntVec(rows, NULL)); /* worst-case, dunno #groups */
    headmap = SeqAsCol(NewIntVec(rows, &hmap)); /* same: don't know #groups */
    linkmap = SeqAsCol(NewIntVec(rows, &lmap));

    InitHashInfo(info, view, mapcol, HashVector(rows), HashValues(view));

    for (r = 0; r < rows; ++r) {
        g = HashFind(view, r, info->hashes[r], info);
        if (g < 0)
            g = info->fill - 1;
        lmap[r] = hmap[g] - 1;
        hmap[g] = r + 1;
    }
    
    /* TODO: reclaim unused entries at end of map and hvec */
    info->mapcol.seq->count = info->veccol.seq->count = info->fill;
    
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

View_p GroupCol (View_p view, Column cols, const char *name) {
    View_p vkey, vres, gview;
    HashInfo info;
    
    vkey = ColMapView(view, cols);
    vres = ColMapView(view, OmitColumn(cols, ViewWidth(view)));

    FillGroupInfo(&info, vkey);
    gview = GroupedView(vres, info.veccol, info.hashcol, name);
    return PairView(RemapSubview(vkey, info.mapcol, 0, -1), gview);
}

static void FillJoinInfo (HashInfo_p info, View_p left, View_p right) {
    int g, r, gleft, nleft, nright, nused = 0, *hmap, *lmap, *jmap;
    Column mapcol, headmap, linkmap, joincol;
    
    nleft = ViewSize(left);
    mapcol = SeqAsCol(NewIntVec(nleft, NULL)); /* worst-case dunno #groups */
    joincol = SeqAsCol(NewIntVec(nleft, &jmap));
    
    InitHashInfo(info, left, mapcol, HashVector(nleft), HashValues(left));

    for (r = 0; r < nleft; ++r) {
        g = HashFind(left, r, info->hashes[r], info);
        if (g < 0)
            g = info->fill - 1;
        jmap[r] = g;
    }

    /* TODO: reclaim unused entries at end of map */
    mapcol.seq->count = info->fill;

    gleft = info->mapcol.seq->count;
    nleft = info->hashcol.seq->count;
    nright = ViewSize(right);
    
    headmap = SeqAsCol(NewIntVec(gleft, &hmap)); /* don't know #groups */
    linkmap = SeqAsCol(NewIntVec(nright, &lmap));

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
    if (info->veccol.seq->count < nused)
        info->veccol = SeqAsCol(NewIntVec(nused, &info->hvec));
    else 
        info->veccol.seq->count = nused;

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

Column IntersectMap (View_p keys, View_p view) {
    int r, rows;
    HashInfo info;
    struct Buffer buffer;
    
    if (!ViewCompat(keys, view)) 
        return SeqAsCol(NULL);
        
    FillHashInfo(&info, view);
    InitBuffer(&buffer);
    rows = ViewSize(keys);
    
    /* these ints are added in increasing order, could have used a bitmap */
    for (r = 0; r < rows; ++r)
        if (HashFind(keys, r, RowHash(keys, r), &info) >= 0)
            ADD_INT_TO_BUF(buffer, r);

    return SeqAsCol(BufferAsIntVec(&buffer));
}

/* ReverseIntersectMap returns RHS indices, instead of IntersectMap's LHS */
static Column ReverseIntersectMap (View_p keys, View_p view) {
    int r, rows;
    HashInfo info;
    struct Buffer buffer;
    
    FillHashInfo(&info, view);
    InitBuffer(&buffer);
    rows = ViewSize(keys);
    
    for (r = 0; r < rows; ++r) {
        int f = HashFind(keys, r, RowHash(keys, r), &info);
        if (f >= 0)
            ADD_INT_TO_BUF(buffer, f);
    }
    
    return SeqAsCol(BufferAsIntVec(&buffer));
}

View_p JoinView (View_p left, View_p right, const char *name) {
    View_p lmeta, rmeta, lkey, rkey, rres, gview;
    Column lmap, rmap;
    HashInfo info;
    
    lmeta = V_Meta(left);
    rmeta = V_Meta(right);
    
    lmap = IntersectMap(lmeta, rmeta);
    /* TODO: optimize, don't create the hash info twice */
    rmap = ReverseIntersectMap(lmeta, rmeta);

    lkey = ColMapView(left, lmap);
    rkey = ColMapView(right, rmap);
    rres = ColMapView(right, OmitColumn(rmap, ViewWidth(right)));

    FillJoinInfo(&info, lkey, rkey);
    gview = GroupedView(rres, info.veccol, info.hashcol, name);
    return PairView(left, RemapSubview(gview, info.mapcol, 0, -1));
}

Column UniqMap (View_p view) {
    HashInfo info;
    FillHashInfo(&info, view);
    return info.mapcol;
}

int HashDoFind (View_p view, int row, View_p w, Column a, Column b, Column c) {
    HashInfo info;
    /* TODO: avoid Log2bits call in InitHashInfo, since done on each find */
    InitHashInfo(&info, w, a, b, c);
    return HashFind(view, row, RowHash(view, row), &info);
}

Column GetHashInfo (View_p left, View_p right, int type) {
    HashInfo info;
    Seq_p seqvec[3];

    switch (type) {
        case 0:     FillHashInfo(&info, left); break;
        case 1:     FillGroupInfo(&info, left); break;
        default:    FillJoinInfo(&info, left, right); break;
    }
        
    seqvec[0] = info.mapcol.seq;
    seqvec[1] = info.veccol.seq;
    seqvec[2] = info.hashcol.seq;

    return SeqAsCol(NewSeqVec(IT_column, seqvec, 3));
}

View_p IjoinView (View_p left, View_p right) {
    View_p view = JoinView(left, right, "?");
    return UngroupView(view, ViewWidth(view)-1);
}

static struct SeqType ST_HashMap = { "hashmap", NULL, 02 };

Seq_p HashMapNew (void) {
    Seq_p result;
    
    result = NewSequence(0, &ST_HashMap, 0);
    /* data[0] is the key + value + hash vector */
    /* data[1] is the allocated count */
    result->data[0].p = NULL;
    result->data[1].i = 0;
    
    return result;
}

/* FIXME: dumb linear scan instead of hashing for now, for small tests only */

static int HashMapLocate(Seq_p hmap, int key, int *pos) {
    const int *data = hmap->data[0].p;
    
    for (*pos = 0; *pos < hmap->count; ++*pos)
        if (key == data[*pos])
            return 1;
        
    return 0;
}

int HashMapAdd (Seq_p hmap, int key, int value) {
    int pos, *data = hmap->data[0].p;
    int allocnt = hmap->data[1].i;

    if (HashMapLocate(hmap, key, &pos)) {
        data[pos+allocnt] = value;
        return 0;
    }

    Assert(pos == hmap->count);
    if (pos >= allocnt) {
        int newlen = (allocnt / 2) * 3 + 10;
        hmap->data[0].p = data = realloc(data, newlen * 2 * sizeof(int));
        memmove(data+newlen, data+allocnt, allocnt * sizeof(int));
        allocnt = hmap->data[1].i = newlen;
    }
    
    data[pos] = key;
    data[pos+allocnt] = value;
    ++hmap->count;
    return allocnt;
}

int HashMapLookup (Seq_p hmap, int key, int defval) {
    int pos;

    if (HashMapLocate(hmap, key, &pos)) {
        int allocnt = hmap->data[1].i;
        const int *data = hmap->data[0].p;
        return data[pos+allocnt];
    }
    
    return defval;
}

#if 0 /* not yet */
int HashMapRemove (Seq_p hmap, int key) {
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
