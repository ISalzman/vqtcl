/*  Implementation of some virtual views.
    $Id$
    This file is part of Vlerq, see src/vlerq.h for full copyright notice. */

static vqView MakeMetaSubview (const char *name, vqView view) {
    vqView meta, result;
    vqVeq names, types, subvs;
    
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

static vqType GroupedGetter (int row, vqCell *item) {
    vqVeq seq = item->c.seq;
    vqView *subviews = seq->GV_cache->data[0].p;
    
    if (subviews[row] == NULL) {
        const int *sptr = seq->GV_start->data[0].p;
        vqVeq gmap = seq->GV_group;
        int start = row > 0 ? sptr[row-1] : 0;
        
        item->c.seq = gmap;
        subviews[row] = vq_incref(RemapSubview(seq->GV_parent, item->c,
                                                    start, sptr[row] - start));
    }
    
    item->v = subviews[row];
    return IT_view;
}

static vqDispatch ST_Grouped = { "grouped", GroupedGetter, 01111 };

vqView GroupedView (vqView view, vqCell startcol, vqCell groupcol, const char *name) {
    int groups;
    vqVeq seq, subviews;
    
    groups = startcol.seq->count;
    subviews = NewSeqVec(IT_view, NULL, groups);

    seq = NewSequence(groups, &ST_Grouped, 0);
    /* data[0] is the parent view to which the grouping applies */
    /* data[1] is the start map, as a sequence */
    /* data[2] is the group map, as a sequence */
    /* data[3] is a cache of subviews, as a pointer vector in a sequence */
    seq->GV_parent = vq_incref(view);
    seq->GV_start = vq_incref(startcol.seq);
    seq->GV_group = vq_incref(groupcol.seq);
    seq->GV_cache = vq_incref(subviews);

    return IndirectView(MakeMetaSubview(name, view), seq);
}

#define UV_parent       data[0].q
#define UV_unmap        data[1].q
#define UV_subcol       data[2].i
#define UV_swidth       data[3].i

static vqType UngroupGetter (int row, vqCell *item) {
    int col, subcol, parentrow;
    const int *data;
    vqView view;
    vqVeq seq;

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

static vqDispatch ST_Ungroup = { "ungroup", UngroupGetter, 011 };

vqView UngroupView (vqView view, int col) {
    int i, n, r, rows;
    struct Buffer buffer;
    vqView subview, meta, submeta, newmeta;
    vqVeq seq, map;
    vqCell column;
    
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
    seq->UV_parent = vq_incref(view);
    seq->UV_unmap = vq_incref(map);
    seq->UV_subcol = col;
    seq->UV_swidth = ViewSize(submeta);
    
    return IndirectView(newmeta, seq);
}

#define BV_parent       data[0].q
#define BV_cumcnt       data[1].q

static vqType BlockedGetter (int row, vqCell *item) {
    int block;
    const int* data;
    vqView subv;
    vqVeq seq;

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

static vqDispatch ST_Blocked = { "blocked", BlockedGetter, 011 };

vqView BlockedView (vqView view) {
    int r, rows, *limits, tally = 0;
    vqView submeta;
    vqVeq seq, offsets;
    vqCell blocks;

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
    seq->BV_parent = vq_incref(view);
    seq->BV_cumcnt = vq_incref(offsets);
    
    submeta = GetViewItem(V_Meta(view), 0, MC_subv, IT_view).v;
    return IndirectView(submeta, seq);
}
