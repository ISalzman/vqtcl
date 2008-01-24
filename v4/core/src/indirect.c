/*
 * indirect.c - Implementation of some virtual views.
 */

#include <stdlib.h>
#include <string.h>

#include "intern.h"
#include "wrap_gen.h"

#define RV_parent   data[0].q
#define RV_map      data[1].q
#define RV_start    data[2].i

static ItemTypes RemapGetter (int row, Item_p item) {
    const int* data;
    Seq_p seq;

    Assert(item->c.pos >= 0);
    seq = item->c.seq;
    data = seq->RV_map->data[0].p;

    item->c = ViewCol(seq->RV_parent, item->c.pos);
    row += seq->RV_start;
    if (data[row] < 0)
        row += data[row];
    return GetItem(data[row], item);
}

static struct SeqType ST_Remap = { "remap", RemapGetter, 011 };

View_p RemapSubview (View_p view, Column mapcol, int start, int count) {
    Seq_p seq;

    if (mapcol.seq == NULL)
        return NULL;
        
    if (count < 0)
        count = mapcol.seq->count;
    
    if (ViewWidth(view) == 0)
        return NoColumnView(count);
        
    seq = NewSequence(count, &ST_Remap, 0);
    /* data[0] is the parent view to which the map applies */
    /* data[1] is the map, as a sequence */
    /* data[2] is the index offset */
    seq->RV_parent = IncRefCount(view);
    seq->RV_map = IncRefCount(mapcol.seq);
    seq->RV_start = start;

    return IndirectView(V_Meta(view), seq);
}

static View_p MakeMetaSubview (const char *name, View_p view) {
    View_p meta, result;
    Seq_p names, types, subvs;
    
    names = NewStrVec(1);
    AppendToStrVec(name, -1, names);

    types = NewStrVec(1);
    AppendToStrVec("V", -1, types);
    
    meta = V_Meta(view);
    subvs = NewSeqVec(IT_view, &meta, 1);
    
    result = NewView(V_Meta(meta));
    SetViewSeqs(result, MC_name, 1, FinishStrVec(names));
    SetViewSeqs(result, MC_type, 1, FinishStrVec(types));
    SetViewSeqs(result, MC_subv, 1, subvs);
    return result;
}

#define GV_parent       data[0].q
#define GV_start        data[1].q
#define GV_group        data[2].q
#define GV_cache        data[3].q

static ItemTypes GroupedGetter (int row, Item_p item) {
    Seq_p seq = item->c.seq;
    View_p *subviews = seq->GV_cache->data[0].p;
    
    if (subviews[row] == NULL) {
        const int *sptr = seq->GV_start->data[0].p;
        Seq_p gmap = seq->GV_group;
        int start = row > 0 ? sptr[row-1] : 0;
        
        item->c.seq = gmap;
        subviews[row] = IncRefCount(RemapSubview(seq->GV_parent, item->c,
                                                    start, sptr[row] - start));
    }
    
    item->v = subviews[row];
    return IT_view;
}

static struct SeqType ST_Grouped = { "grouped", GroupedGetter, 01111 };

View_p GroupedView (View_p view, Column startcol, Column groupcol, const char *name) {
    int groups;
    Seq_p seq, subviews;
    
    groups = startcol.seq->count;
    subviews = NewSeqVec(IT_view, NULL, groups);

    seq = NewSequence(groups, &ST_Grouped, 0);
    /* data[0] is the parent view to which the grouping applies */
    /* data[1] is the start map, as a sequence */
    /* data[2] is the group map, as a sequence */
    /* data[3] is a cache of subviews, as a pointer vector in a sequence */
    seq->GV_parent = IncRefCount(view);
    seq->GV_start = IncRefCount(startcol.seq);
    seq->GV_group = IncRefCount(groupcol.seq);
    seq->GV_cache = IncRefCount(subviews);

    return IndirectView(MakeMetaSubview(name, view), seq);
}

#define PV_parent   data[0].q
#define PV_start    data[1].i
#define PV_rate     data[2].i
#define PV_step     data[3].i

static ItemTypes StepGetter (int row, Item_p item) {
    Seq_p seq = item->c.seq;
    int rows = ViewSize(seq->PV_parent);

    item->c = ViewCol(seq->PV_parent, item->c.pos);
    row = (seq->PV_start + (row / seq->PV_rate) * seq->PV_step) % rows;
    if (row < 0)
        row += rows;
    return GetItem(row, item);
}

static struct SeqType ST_Step = { "step", StepGetter, 01 };

View_p StepView (View_p view, int count, int offset, int rate, int step) {
    Seq_p seq;

    /* prevent division by zero if input view is empty */
    if (ViewSize(view) == 0)
        return view;

    seq = NewSequence(count * rate, &ST_Step, 0);
    /* data[0] is the parent view to which the changes apply */
    /* data[1] is the starting offset */
    /* data[2] is the rate to repeat an item */
    /* data[3] is the step to the next item */
    seq->PV_parent = IncRefCount(view);
    seq->PV_start = offset;
    seq->PV_rate = rate;
    seq->PV_step = step;

    return IndirectView(V_Meta(view), seq);
}

#define CV_left     data[0].q
#define CV_right    data[1].q

static ItemTypes ConcatGetter (int row, Item_p item) {
    View_p view = item->c.seq->CV_left;
    int rows = ViewSize(view);
    
    if (row >= rows) {
        row -= rows;
        view = item->c.seq->CV_right;
    }

    item->c = ViewCol(view, item->c.pos);
    return GetItem(row, item);
}

static struct SeqType ST_Concat = { "concat", ConcatGetter, 011 };

View_p ConcatView (View_p view1, View_p view2) {
    int rows1, rows2;
    Seq_p seq;

    if (view1 == NULL || view2 == NULL || !ViewCompat(view1, view2))
        return NULL;
                    
    rows1 = ViewSize(view1);
    if (rows1 == 0)
        return view2;

    rows2 = ViewSize(view2);
    if (rows2 == 0)
        return view1;

    seq = NewSequence(rows1 + rows2, &ST_Concat, 0);
    /* data[0] is the left-hand view */
    /* data[1] is the right-hand view */
    seq->CV_left = IncRefCount(view1);
    seq->CV_right = IncRefCount(view2);

    return IndirectView(V_Meta(view1), seq);
}

#define UV_parent       data[0].q
#define UV_unmap        data[1].q
#define UV_subcol       data[2].i
#define UV_swidth       data[3].i

static ItemTypes UngroupGetter (int row, Item_p item) {
    int col, subcol, parentrow;
    const int *data;
    View_p view;
    Seq_p seq;

    col = item->c.pos;
    seq = item->c.seq;

    view = seq->UV_parent;
    data = seq->UV_unmap->data[0].p;
    subcol = seq->UV_subcol;

    parentrow = data[row];
    if (parentrow < 0) {
        parentrow = data[row+parentrow];
        row = -data[row];
    } else
        row = 0;
    
    if (subcol <= col && col < subcol + seq->UV_swidth) {
        view = GetViewItem(view, parentrow, subcol, IT_view).v;
        col -= subcol;
    } else {
        if (col >= subcol)
            col -= seq->UV_swidth - 1;
        row = parentrow;
    }
    
    item->c = ViewCol(view, col);
    return GetItem(row, item);
}

static struct SeqType ST_Ungroup = { "ungroup", UngroupGetter, 011 };

View_p UngroupView (View_p view, int col) {
    int i, n, r, rows;
    struct Buffer buffer;
    View_p subview, meta, submeta, newmeta;
    Seq_p seq, map;
    Column column;
    
    InitBuffer(&buffer);

    column = ViewCol(view, col);
    rows = column.seq->count;
    
    for (r = 0; r < rows; ++r) {
        subview = GetColItem(r, column, IT_view).v;
        n = ViewSize(subview);
        if (n > 0) {
            ADD_INT_TO_BUF(buffer, r);
            for (i = 1; i < n; ++i)
                ADD_INT_TO_BUF(buffer, -i);
        }
    }

    map = BufferAsIntVec(&buffer);
    
    /* result meta view replaces subview column with its actual meta view */
    meta = V_Meta(view);
    submeta = GetViewItem(meta, col, MC_subv, IT_view).v;
    newmeta = ConcatView(FirstView(meta, col), submeta);
    newmeta = ConcatView(newmeta, LastView(meta, ViewSize(meta) - (col + 1)));
    
    seq = NewSequence(map->count, &ST_Ungroup, 0);
    /* data[0] is the parent view */
    /* data[1] is ungroup map as a sequence */
    /* data[2] is the subview column */
    /* data[3] is the subview width */
    seq->UV_parent = IncRefCount(view);
    seq->UV_unmap = IncRefCount(map);
    seq->UV_subcol = col;
    seq->UV_swidth = ViewSize(submeta);
    
    return IndirectView(newmeta, seq);
}

#define BV_parent       data[0].q
#define BV_cumcnt       data[1].q

static ItemTypes BlockedGetter (int row, Item_p item) {
    int block;
    const int* data;
    View_p subv;
    Seq_p seq;

    seq = item->c.seq;
    data = seq->BV_cumcnt->data[0].p;

    for (block = 0; block + data[block] < row; ++block)
        ;

    if (row == block + data[block]) {
        row = block;
        block = ViewSize(seq->BV_parent) - 1;
    } else if (block > 0)
        row -= block + data[block-1];

    subv = GetViewItem(seq->BV_parent, block, 0, IT_view).v;
    item->c = ViewCol(subv, item->c.pos);    
    return GetItem(row, item);
}

static struct SeqType ST_Blocked = { "blocked", BlockedGetter, 011 };

View_p BlockedView (View_p view) {
    int r, rows, *limits, tally = 0;
    View_p submeta;
    Seq_p seq, offsets;
    Column blocks;

    /* view must have exactly one subview column */
    if (ViewWidth(view) != 1)
        return NULL;
    
    blocks = ViewCol(view, 0);
    rows = ViewSize(view);
    
    offsets = NewIntVec(rows, &limits);
    for (r = 0; r < rows; ++r) {
        tally += ViewSize(GetColItem(r, blocks, IT_view).v);
        limits[r] = tally;
    }
    
    seq = NewSequence(tally, &ST_Blocked, 0);
    /* data[0] is the parent view */
    /* data[1] is a cumulative row count, as a sequence */
    seq->BV_parent = IncRefCount(view);
    seq->BV_cumcnt = IncRefCount(offsets);
    
    submeta = GetViewItem(V_Meta(view), 0, MC_subv, IT_view).v;
    return IndirectView(submeta, seq);
}
