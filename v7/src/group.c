/*  Implementation of some virtual views.
    $Id$
    This file is part of Vlerq, see src/vlerq.h for full copyright notice. */

static vqView MakeMetaSubview (const char *name, vqView view) {
    vqView result;
    result = vq_new(vwMeta(empty_meta(vwState(view))), 1);
    vq_setMetaRow(result, 0, name, VQ_view, vwMeta(view));
    return result;
}

#define GV_parent       data[0].q
#define GV_start        data[1].q
#define GV_group        data[2].q
#define GV_cache        data[3].q

static vqType GroupedGetter (int row, vqCell *item) {
#if 0
    vqVec seq = item->p;
    vqView *subviews = seq->GV_cache->data[0].p;
    
    if (subviews[row] == 0) {
        const int *sptr = seq->GV_start->data[0].p;
        vqVec gmap = seq->GV_group;
        int start = row > 0 ? sptr[row-1] : 0;
        
        item->p = gmap;
        subviews[row] = vq_incref(RemapSubview(seq->GV_parent, *item,
                                                    start, sptr[row] - start));
    }
    
    item->v = subviews[row];
#endif
    return VQ_view;
}

static vqDispatch ST_Grouped = { "grouped", /* 01111 */0, 0, 0, GroupedGetter };

static vqView GroupedView (vqView view, vqVec startcol, vqVec groupcol, const char *name) {
#if 0
    int groups;
    vqVec seq, subviews;
    
    groups = vSize(startcol.p);
    subviews = NewSeqVec(VQ_view, 0, groups);

    seq = NewSequence(groups, &ST_Grouped, 0);
    /* data[0] is the parent view to which the grouping applies */
    /* data[1] is the start map, as a sequence */
    /* data[2] is the group map, as a sequence */
    /* data[3] is a cache of subviews, as a pointer vector in a sequence */
    seq->GV_parent = vq_incref(view);
    seq->GV_start = vq_incref(startcolp);
    seq->GV_group = vq_incref(groupcolp);
    seq->GV_cache = vq_incref(subviews);

    return IndirectView(MakeMetaSubview(name, view), seq);
#endif
    return 0;
}

#define UV_parent       data[0].q
#define UV_unmap        data[1].q
#define UV_subcol       data[2].i
#define UV_swidth       data[3].i

static vqType UngroupGetter (int row, vqCell *item) {
#if 0
    int col, subcol, parentrow;
    const int *data;
    vqView view;
    vqVec seq;

    col = item->x.y.i;
    seq = item->p;

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
        view = vq_getView(view, parentrow, subcol, 0);
        col -= subcol;
    } else {
        if (col >= subcol)
            col -= seq->UV_swidth - 1;
        row = parentrow;
    }
    
    item = vwCol(view, col);
    return getcell(row, item);
#endif
    return VQ_nil;
}

static vqDispatch ST_Ungroup = { "ungroup", 0, 011, 0, UngroupGetter };

static vqView UngroupView (vqView view, int col) {
#if 0
    int i, n, r, rows;
    struct Buffer buffer;
    vqView subview, meta, submeta, newmeta;
    vqVec seq, map;
    vqCell column;
    
    InitBuffer(&buffer);

    column = vwCol(view, col);
    rows = vSize(column.p);
    
    for (r = 0; r < rows; ++r) {
        subview = GetColItem(r, column, VQ_view).v;
        n = vwRows(subview);
        if (n > 0) {
            ADD_INT_TO_BUF(buffer, r);
            for (i = 1; i < n; ++i)
                ADD_INT_TO_BUF(buffer, -i);
        }
    }

    map = BufferAsIntVec(&buffer);
    
    /* result meta view replaces subview column with its actual meta view */
    meta = vwMeta(view);
    submeta = vq_getView(meta, col, MC_subv, 0);
    newmeta = ConcatView(FirstView(meta, col), submeta);
    newmeta = ConcatView(newmeta, LastView(meta, vwRows(meta) - (col + 1)));
    
    seq = NewSequence(vSize(map), &ST_Ungroup, 0);
    /* data[0] is the parent view */
    /* data[1] is ungroup map as a sequence */
    /* data[2] is the subview column */
    /* data[3] is the subview width */
    seq->UV_parent = vq_incref(view);
    seq->UV_unmap = vq_incref(map);
    seq->UV_subcol = col;
    seq->UV_swidth = vwRows(submeta);
    
    return IndirectView(newmeta, seq);
#endif
    return 0;
}

#define BV_parent       data[0].q
#define BV_cumcnt       data[1].q

static vqType BlockedGetter (int row, vqCell *item) {
#if 0
    int block;
    const int* data;
    vqView subv;
    vqVec seq;

    seq = item->p;
    data = seq->BV_cumcnt->data[0].p;

    for (block = 0; block + data[block] < row; ++block)
        ;

    if (row == block + data[block]) {
        row = block;
        block = vwRows(seq->BV_parent) - 1;
    } else if (block > 0)
        row -= block + data[block-1];

    subv = vq_getView(seq->BV_parent, block, 0, 0);
    item = vwCol(subv, item->x.y.i);    
    return getcell(row, item);
#endif
    return VQ_nil;
}

static vqDispatch ST_Blocked = { "blocked", 011, 0, 0, BlockedGetter };

static vqView BlockedView (vqView view) {
#if 0
    int r, rows, *limits, tally = 0;
    vqView submeta;
    vqVec seq, offsets;
    vqCell blocks;

    /* view must have exactly one subview column */
    if (vwCols(view) != 1)
        return 0;
    
    blocks = vwCol(view, 0);
    rows = vwRows(view);
    
    offsets = NewIntVec(rows, &limits);
    for (r = 0; r < rows; ++r) {
        tally += vwRows(GetColItem(r, blocks, VQ_view).v);
        limits[r] = tally;
    }
    
    seq = NewSequence(tally, &ST_Blocked, 0);
    /* data[0] is the parent view */
    /* data[1] is a cumulative row count, as a sequence */
    seq->BV_parent = vq_incref(view);
    seq->BV_cumcnt = vq_incref(offsets);
    
    submeta = vq_getView(vwMeta(view), 0, MC_subv, 0);
    return IndirectView(submeta, seq);
#endif
    return 0;
}
