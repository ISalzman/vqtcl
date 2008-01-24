/*
 * mutable.c - Implementation of mutable views.
 */

#include <stdlib.h>
#include <string.h>

#include "intern.h"
#include "wrap_gen.h"

#define MUT_DEBUG 0

#define S_aux(seq,typ) ((typ) ((Seq_p) (seq) + 1))

#define SV_data(seq)    S_aux(seq, Seq_p*)
#define SV_bits         data[0].q
#define SV_hash         data[1].q
#define SV_view         data[2].q

static void SettableCleaner (Seq_p seq) {
    int c, r;
    View_p view = seq->SV_view;
    
    for (c = 0; c < ViewWidth(view); ++c) {
        Seq_p dseq = SV_data(seq)[c];

        if (dseq != NULL)
            switch (ViewColType(view, c)) {

                case IT_string: {
                    char **p = S_aux(dseq, char**);
                    for (r = 0; r < dseq->count; ++r)
                        if (p[r] != NULL)
                            free(p[r]);
                    break;
                }

                case IT_bytes: {
                    const Item *p = S_aux(dseq, const Item*);
                    for (r = 0; r < dseq->count; ++r)
                        if (p[r].u.ptr != NULL)
                            free((char*) p[r].u.ptr);
                    break;
                }

                case IT_view: {
                    const View_p *p = S_aux(dseq, const View_p*);
                    for (r = 0; r < dseq->count; ++r)
                        if (p[r] != NULL)
                            DecRefCount(p[r]);
                    break;
                }
            
                default: break;
            }

        DecRefCount(dseq);
        DecRefCount(SV_data(seq)[ViewWidth(view)+c]);
    }
}

static ItemTypes SettableGetter (int row, Item_p item) {
    int col = item->c.pos;
    Seq_p seq = item->c.seq;
    View_p view = seq->SV_view;
    
    item->c = ViewCol(view, col);
    
    if (TestBit(seq->SV_bits, row)) {
        int index = HashMapLookup(seq->SV_hash, row, -1);
        Assert(index >= 0);
    
        if (TestBit(SV_data(seq)[ViewWidth(view)+col], index)) {
            row = index;
            item->c.seq = SV_data(seq)[col];
        }
    }
    
    return GetItem(row, item);
}
 
static int SettableWidth (ItemTypes type) {
    switch (type) {
        case IT_int:  case IT_float:    return 4;
        case IT_wide: case IT_double:   return 8;
        case IT_view: case IT_string:   return sizeof(void*);
        case IT_bytes:                  return sizeof(Item);
        default:                        Assert(0); return 0;
    }
}

static ItemTypes StringSetGetter (int row, Item_p item) {
    item->s = S_aux(item->c.seq, const char**)[row];
    return IT_string;
}

static ItemTypes BytesSetGetter (int row, Item_p item) {
    item->u = S_aux(item->c.seq, const Item*)[row].u;
    return IT_bytes;
}

static ItemTypes ViewSetGetter (int row, Item_p item) {
    item->v = S_aux(item->c.seq, const View_p*)[row];
    return IT_view;
}

static struct SeqType ST_StringSet = { "stringset", StringSetGetter };
static struct SeqType ST_BytesSet = { "bytesset", BytesSetGetter };
static struct SeqType ST_ViewSet = { "viewset", ViewSetGetter };

static SeqType_p PickSetGetter (ItemTypes type) {
    int w = 8;
    switch (type) {
        case IT_int:    w = 4; /* fall through */
        case IT_wide:   return FixedGetter(w, 1, 0, 0);
        case IT_float:  w = 4; /* fall through */
        case IT_double: return FixedGetter(w, 1, 1, 0);
        case IT_string: return &ST_StringSet;
        case IT_bytes:  return &ST_BytesSet;
        case IT_view:   return &ST_ViewSet;
        default:        Assert(0); return NULL;
    }
}

static struct SeqType ST_Settable = {
    "settable", SettableGetter, 0111, SettableCleaner
};

static int IsSettable(View_p view) {
    Seq_p seq = ViewCol(view, 0).seq;
    return seq != NULL && seq->type == &ST_Settable;
}

static void SettableSetter (View_p view, int row, int col, Item_p item) {
    int index, count, limit;
    ItemTypes type;
    Seq_p seq, dseq, hash;
    
    Assert(IsSettable(view));
    
    seq = ViewCol(view, col).seq;
    type = ViewColType(view, col);
    dseq = SV_data(seq)[col];
    hash = seq->SV_hash;

    if (SetBit(&seq->SV_bits, row)) {
        index = hash->count;
        HashMapAdd(hash, row, index);
    } else {
        index = HashMapLookup(hash, row, -1);
        Assert(index >= 0);
    }
    
    limit = SetBit(&SV_data(seq)[ViewWidth(view)+col], index);
    count = dseq != NULL ? dseq->count : 0;
    
    /* clumsy: resize a settable column so it can store more items if needed */
    if (limit > count) {
        int w = SettableWidth(type);
        Seq_p nseq;
        nseq = IncRefCount(NewSequence(limit, PickSetGetter(type), limit * w));
        memcpy(nseq + 1, LoseRef(dseq) + 1, count * w);
        SV_data(seq)[col] = dseq = nseq;
    }
    
    switch (type) {
        
        case IT_int:    S_aux(dseq, int*    )[index] = item->i; break;
        case IT_wide:   S_aux(dseq, int64_t*)[index] = item->w; break;
        case IT_float:  S_aux(dseq, float*  )[index] = item->f; break;                                  
        case IT_double: S_aux(dseq, double* )[index] = item->d; break;

        case IT_string: {
            char **p = S_aux(dseq, char**) + index;
            free(*p);
            *p = strdup(item->s);
            break;
        }
        
        case IT_bytes: {
            Item *p = S_aux(dseq, Item*) + index;
            free((char*) p->u.ptr);
            p->u.len = item->u.len;
            p->u.ptr = memcpy(malloc(p->u.len), item->u.ptr, p->u.len);
            break;
        }
        
        case IT_view: {
            View_p *p = S_aux(dseq, View_p*) + index;
            LoseRef(*p);
            *p = IncRefCount(item->v);
            break;
        }
        
        default: Assert(0); break;
    }
}

static View_p SettableView (View_p view) {
    Int_t bytes = 2 * ViewWidth(view) * sizeof(Seq_p*);
    Seq_p seq;
    
    /* room for a set of new data columns, then a set of "used" bitmaps */
    seq = NewSequence(ViewSize(view), &ST_Settable, bytes);
    /* data[0] is the adjusted bitmap */
    /* data[1] points to hash row map */
    /* data[2] points to the original view */
    seq->SV_bits = NULL;
    seq->SV_hash = IncRefCount(HashMapNew());
    seq->SV_view = IncRefCount(view);

    return IndirectView(V_Meta(view), seq);
}    

#define MV_parent       data[0].q
#define MV_mutmap       data[1].q

    typedef struct MutRange {
        int start;
        int shift;
        View_p added;
    } *MutRange_p;

static void MutableCleaner (Seq_p seq) {
    int i;
    Seq_p map;
    MutRange_p range;
    
    map = seq->MV_mutmap;
    range = S_aux(map, MutRange_p);

    for (i = 0; i < map->count; ++i)
        DecRefCount(range[i].added);
}

#if MUT_DEBUG
#include <stdio.h>
static void DumpRanges (const char *msg, View_p view) {
    int i;
    Seq_p seq, map;
    MutRange_p range;

    seq = ViewCol(view, 0).seq;
    map = seq->MV_mutmap;
    range = S_aux(map, MutRange_p);

    printf("%s: view %p rc %d, has %d rows %d ranges\n",
                 msg, (void*) view, view->refs, ViewSize(view), map->count);
    for (i = 0; i < map->count; ++i) {
        View_p v = range[i].added;
        printf("    %2d: st %3d sh %3d", i, range[i].start, range[i].shift);
        if (v != NULL)
            printf(" - %p: sz %3d rc %3d\n", (void*) v, ViewSize(v), v->refs);
        printf("\n");
    }
}
#else
#define DumpRanges(a,b)
#endif

static int MutSlot (MutRange_p range, int pos) {
    int i = -1;
    /* TODO: use binary i.s.o. linear search */
    while (pos > range[++i].start)
        ;
    return pos < range[i].start ? i-1 : i;
}

static int ChooseMutView (Seq_p seq, int *prow) {
    int slot, row = *prow;
    MutRange_p range;
    
    range = S_aux(seq->MV_mutmap, MutRange_p);
    
    slot = MutSlot(range, row);
    row -= range[slot].start;
    Assert(row >= 0);

    *prow = range[slot].added != NULL ? slot : -1;
    return row + range[slot].shift;
}

static ItemTypes MutableGetter (int row, Item_p item) {
    Seq_p seq = item->c.seq;
    View_p view = seq->MV_parent;
    int index = ChooseMutView(seq, &row);

    if (row >= 0) {
        MutRange_p range = S_aux(seq->MV_mutmap, MutRange_p);
        view = range[row].added;
    }

    item->c = ViewCol(view, item->c.pos);
    return GetItem(index, item);
}

static MutRange_p MutResize (Seq_p *seqp, int pos, int diff) {
    int i, limit = 0, count = 0;
    MutRange_p range = NULL;
    Seq_p map;
    
    map = *seqp;
    if (map != NULL) {
        count = map->count;
        limit = map->data[1].i / sizeof(struct MutRange);
        range = S_aux(map, MutRange_p);
    }
    
    /* TODO: this only grows the ranges, should also shrink when possible */
    if (diff > 0 && count + diff > limit) {
        int newlimit;
        MutRange_p newrange;
        
        newlimit = (count / 2) * 3 + 5;
        Assert(pos + diff <= newlimit);
            
        *seqp = NewSequence(count, PickIntGetter(0),
                                newlimit * sizeof(struct MutRange));

        newrange = S_aux(*seqp, MutRange_p);
        memcpy(newrange, range, count * sizeof(struct MutRange));
        range = newrange;

        LoseRef(map);
        map = IncRefCount(*seqp);
    }
    
    map->count += diff;
    
    if (diff > 0) {
        for (i = count; --i >= pos; )
            range[i+diff] = range[i];
    } else if (diff < 0) {
        for (i = pos; i < pos - diff; ++i)
            LoseRef(range[i].added);
        for (i = pos - diff; i < count; ++i)
            range[i+diff] = range[i];
    }
    
    return range;
}

static struct SeqType ST_Mutable = {
    "mutable", MutableGetter, 011, MutableCleaner 
};

View_p MutableView (View_p view) {
    int rows;
    Seq_p seq;

    rows = ViewSize(view);
    seq = NewSequence(rows, &ST_Mutable, 0);            
    /* data[0] is the parent view (may be changed to a settable one later) */
    /* data[1] holds the mutable range map, as a sequence */
    seq->MV_parent = IncRefCount(view);
    seq->MV_mutmap = NULL;
    
    if (rows == 0)
        MutResize(&seq->MV_mutmap, 0, 1);
    else {
        MutRange_p range = MutResize(&seq->MV_mutmap, 0, 2);
        range[1].start = rows;
    }
    
    return IndirectView(V_Meta(view), seq);
}

int IsMutable (View_p view) {
    Seq_p seq = ViewCol(view, 0).seq;
    return seq != NULL && seq->type == &ST_Mutable;
}

View_p ViewSet (View_p view, int row, int col, Item_p item) {
    int index;
    View_p *modview;
    Seq_p mutseq;

    if (!IsMutable(view))
        view = MutableView(view);
    
    mutseq = ViewCol(view, 0).seq;
    index = ChooseMutView(mutseq, &row);

    if (row >= 0) {
        MutRange_p range = S_aux(mutseq->MV_mutmap, MutRange_p);
        modview = &range[row].added;
    } else
        modview = (View_p*) &mutseq->data[0].p;
    
    if (!IsSettable(*modview))
        *modview = IncRefCount(SettableView(LoseRef(*modview)));
        
    SettableSetter(*modview, index, col, item);

    return view;
}

View_p ViewReplace (View_p view, int offset, int count, View_p data) {
    int drows, fend, fpos, fslot, tend, tpos, tslot, grow, diff;
    Seq_p seq, map;
    MutRange_p range;
    View_p tailv;
    
    drows = data != NULL ? ViewSize(data) : 0;
    fend = offset + count;
    tend = offset + drows;

    if (drows > 0 && !ViewCompat(view, data))
        return NULL;

    if (drows == 0 && count == 0)
        return view;
        
    if (!IsMutable(view))
        view = MutableView(view);

    DumpRanges("before", view);

    seq = ViewCol(view, 0).seq;
    map = seq->MV_mutmap;
    range = S_aux(map, MutRange_p);
    
    fslot = MutSlot(range, offset);
    fpos = offset - range[fslot].start;
    
    tslot = MutSlot(range, fend);
    tpos = fend - range[tslot].start;
    
    tailv = range[tslot].added;

#if MUT_DEBUG
printf("fslot %d fpos %d tslot %d tpos %d drows %d offset %d count %d fend %d tend %d\n", fslot,fpos,tslot,tpos,drows,offset,count,fend,tend);
#endif

    /* keep head of first range, if rows remain before offset */
    if (fpos > 0)
        ++fslot;
    
    grow = fslot - tslot;
    if (drows > 0)
        ++grow;

    range = MutResize(&seq->MV_mutmap, fslot, grow);
    tslot += grow;
    
    if (fpos > 0 && grow > 0)
        range[tslot].shift = range[fslot-1].shift;
    
    if (drows > 0) {
        range[fslot].start = offset;
        range[fslot].shift = 0;
        range[fslot].added = IncRefCount(data);
    }
        
    /* keep tail of last range, if rows remain after offset */
    if (tpos > 0) {
        range[tslot].start = tend;
        range[tslot].shift += tpos;
        LoseRef(range[tslot].added);
        range[tslot].added = IncRefCount(tailv);
        ++tslot;
    }
    
    diff = drows - count;
    seq->count += diff;
    
    map = seq->MV_mutmap;
    while (tslot < map->count)
        range[tslot++].start += diff;

    DumpRanges("after", view);
    return view;
}

ItemTypes SetCmd_MIX (Item args[]) {
    int i, col, row, objc;
    const Object_p *objv;
    View_p view;
    Item item;
    
    view = ObjAsView(args[0].o);
    row = args[1].i;
    if (row < 0)
        row += ViewSize(view);
    
    objv = (const void*) args[2].u.ptr;
    objc = args[2].u.len;

    if (objc % 2 != 0) {
        args->e = EC_wnoa;
        return IT_error;
    }

    for (i = 0; i < objc; i += 2) {
        col = ColumnByName(V_Meta(view), objv[i]);
        if (col < 0)
            return IT_unknown;

        item.o = objv[i+1];
        if (!ObjToItem(ViewColType(view, col), &item))
            return IT_unknown;

        view = ViewSet(view, row, col, &item);
    }

    args->v = view;
    return IT_view;
}

Seq_p MutPrepare (View_p view) {
    int i, j, k, cols, ranges, parows, delcnt, delpos, count, adjpos;
    Seq_p result, *seqs, mutseq, mutmap;
    MutRange_p range, rp;
    View_p parent, insview;
    Column column;
    
    Assert(IsMutable(view));
    
    mutseq = ViewCol(view, 0).seq;
    parent = mutseq->MV_parent;
    parows = ViewSize(parent);
    mutmap = mutseq->MV_mutmap;
    range = S_aux(mutmap, MutRange_p);
    ranges = mutmap->count - 1;
    
    cols = ViewWidth(view);
    result = NewSeqVec(IT_unknown, NULL, MP_limit);
    
    seqs = S_aux(result, Seq_p*);
        
    seqs[MP_delmap] = NewBitVec(parows);
    seqs[MP_insmap] = NewBitVec(ViewSize(view));

    delcnt = delpos = 0;
    insview = CloneView(view);
    
    for (i = 0; i < ranges; ++i) {
        rp = range + i;
        count = (rp+1)->start - rp->start;
        if (rp->added == NULL) {
            delcnt += rp->shift - delpos;
            SetBitRange(seqs[MP_delmap], delpos, rp->shift - delpos);
            delpos = rp->shift + count;
        } else {
            View_p newdata = StepView(rp->added, count, rp->shift, 1, 1);
            insview = ViewReplace(insview, ViewSize(insview), 0, newdata);
            SetBitRange(seqs[MP_insmap], rp->start, count);
        }
    }
    
    delcnt += parows - delpos;
    SetBitRange(seqs[MP_delmap], delpos, parows - delpos);
    seqs[MP_insdat] = insview;

    seqs[MP_adjmap] = NewBitVec(parows - delcnt);

    if (IsSettable(parent)) {
        Seq_p setseq, adjseq;
        
        setseq = ViewCol(parent, 0).seq;
        adjseq = setseq->SV_bits;
                
        if (adjseq != NULL && adjseq->count > 0) {
            int *revptr, *rowptr;
            Seq_p revmap, rowmap, hash = setseq->SV_hash;
            View_p bitmeta, bitview;
        
            revmap = NewIntVec(hash->count, &revptr);
            rowmap = NewIntVec(hash->count, &rowptr);

            k = adjpos = 0;
            
            for (i = 0; i < ranges; ++i) {
                rp = range + i;
                if (rp->added == NULL) {
                    count = (rp+1)->start - rp->start;
                    for (j = 0; j < count; ++j)
                        if (TestBit(adjseq, rp->start + j)) {
                            int index = HashMapLookup(hash, rp->start + j, -1);
                            Assert(index >= 0);
                            revptr[k] = index;
                            rowptr[k++] = rp->start + j;
                            SetBit(seqs+MP_adjmap, adjpos + j);
                        }
                    adjpos += count;
                }
            }
            
            Assert(adjpos == parows - delcnt);
            
            seqs[MP_adjdat] = RemapSubview(parent, SeqAsCol(rowmap), 0, -1);

            bitmeta = StepView(MakeIntColMeta("_"), 1, 0, ViewWidth(parent), 1);
            bitview = NewView(bitmeta); /* will become an N-intcol view */
            
            for (i = 0; i < cols; ++i) {
                column.seq = SV_data(setseq)[cols+i];
                column.pos = -1;
                MinBitCount(&column.seq, hash->count);
                SetViewCols(bitview, i, 1, column);
            }

            seqs[MP_revmap] = revmap;
            seqs[MP_usemap] = RemapSubview(bitview, SeqAsCol(revmap), 0, -1);
        }
    } else {
        seqs[MP_adjdat] = seqs[MP_usemap] = NoColumnView(0);
        seqs[MP_revmap] = ViewCol(seqs[MP_usemap], 0).seq;
    }
    
    for (i = 0; i < MP_limit; ++i)
        IncRefCount(seqs[i]);       
         
    return result;
}
