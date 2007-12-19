/*  Additional view operators.
    $Id$
    This file is part of Vlerq, see base/vlerq.h for full copyright notice. */

#include "vq_conf.h"

#if VQ_OPDEF_H

static vq_Type StepGetter (int row, vq_Cell *item) {
    Vector v = vData(item->o.a.v);
    item->o.a.i = v[0].o.a.i + v[0].o.b.i * (row / v[1].o.a.i);
    return VQ_int;
}

static Dispatch steptab = {
    "step", 3, 0, 0, IndirectCleaner, StepGetter
};

vq_View StepVop (int rows, int start, int step, int rate, const char *name) {
    vq_View v, meta = vq_new(1, vq_meta(0));
    Vq_setMetaRow(meta, 0, name != NULL ? name : "", VQ_int, NULL);
    v = IndirectView(meta, &steptab, rows, 2 * sizeof(vq_Cell));
    vData(v)[0].o.a.i = start;
    vData(v)[0].o.b.i = rate > 0 ? step : 1;
    vData(v)[1].o.a.i = rate > 0 ? rate : 1;
    return v;
}

vq_View PassVop (vq_View v) {
    vq_View t = vq_new(0, vMeta(v));
    int c, cols = vCount(vMeta(v));
    vCount(t) = vCount(v);
    for (c = 0; c < cols; ++c) {
        t[c] = v[c];
        vq_retain(t[c].o.a.v);
    }
    return t;
}

const char* TypeVop (vq_View v) {
    return vType(v)->name;
}

vq_View ViewVop (vq_View v, vq_View m) {
    assert(v != NULL);
    /* TODO: on-the-fly restructuring i.s.o. always doing a vq_new */
    if (m != NULL)
        v = vq_new(vCount(v), m);
    return v;
}

static void RowColMapCleaner (vq_View v) {
    vq_release(vOrig(v));
    vq_release(vData(v)->o.a.v);
    IndirectCleaner(v);
}

static vq_Type RowMapGetter (int row, vq_Cell *itemp) {
    int n;
    vq_View v = itemp->o.a.v;
    vq_Cell map = *vData(v);
    if (map.o.a.v != NULL && GetItem(row, &map) == VQ_int) {
        /* extra logic to support special maps with relative offsets < 0 */
        if (map.o.a.i < 0) {
            row += map.o.a.i;
            map = *vData(v);
            GetItem(row, &map);
        }
        row = map.o.a.i;
    }
    v = vOrig(v);
    n = vCount(v);
    if (row >= n && n > 0)
        row %= n;
    *itemp = v[itemp->o.b.i];
    return GetItem(row, itemp);
}

static Dispatch rowmaptab = {
    "rowmap", 3, 0, 0, RowColMapCleaner, RowMapGetter
};

vq_View RowMapVop (vq_View v, vq_View map) {
    vq_View t, m = vMeta(map);
    t = IndirectView(vMeta(v), &rowmaptab, vCount(map), sizeof(vq_Cell));
    vOrig(t) = vq_retain(v);
    if (vCount(m) > 0 && (Vq_getInt(m, 0, 1, VQ_nil) & VQ_TYPEMASK) == VQ_int) {
        *vData(t) = map[0];
        vq_retain(map[0].o.a.v);
    }
    return t;
}

static vq_Type ColMapGetter (int row, vq_Cell *itemp) {
    int n;
    vq_View v = itemp->o.a.v;
    int col = itemp->o.b.i;
    vq_Cell map = *vData(v);
    if (map.o.a.v != NULL && GetItem(col, &map) == VQ_int)
        col = map.o.a.i;
    v = vOrig(v);
    n = vCount(vMeta(v));
    if (col >= n && n > 0)
        col %= n;
    *itemp = v[col];
    return GetItem(row, itemp);
}

static Dispatch colmaptab = {
    "colmap", 3, 0, 0, RowColMapCleaner, ColMapGetter
};

vq_View ColMapVop (vq_View v, vq_View map) {
    /* TODO: better - store col refs in a new view, avoids extra getter layer */
    vq_View t, m = vMeta(map), mm = RowMapVop(vMeta(v), map);
    t = IndirectView(mm, &colmaptab, vCount(v), sizeof(vq_Cell));
    vOrig(t) = vq_retain(v);
    if (vCount(m) > 0 && (Vq_getInt(m, 0, 1, VQ_nil) & VQ_TYPEMASK) == VQ_int) {
        *vData(t) = map[0];
        vq_retain(map[0].o.a.v);
    }
    return t;
}

static void RowCatCleaner (vq_View v) {
    int n = vOffs(v);
    while (--n >= 0)
        vq_release(vData(v)[n].o.a.v);
    IndirectCleaner(v);
}

static vq_Type RowCatGetter (int row, vq_Cell *itemp) {
    vq_Cell *v = vData(itemp->o.a.v);
    while (row >= v->o.b.i)
        row -= (v++)->o.b.i;
    *itemp = v->o.a.v[itemp->o.b.i];
    return GetItem(row, itemp);
}

static Dispatch rowcattab = {
    "rowcat", 3, 0, 0, RowCatCleaner, RowCatGetter
};

vq_View RowCatVop (Vector views) {
    vq_View t;
    vq_Cell item;
    Vector data;
    int i, n = vCount(views);
    vq_retain(views);
    assert(n > 0);
    t = IndirectView(vMeta(views[0].o.a.v), &rowcattab, 0, n * sizeof(vq_Cell));
    data = vData(t);
    for (i = 0; i < n; ++i) {
        item.o.a.v = views;
        GetItem(i, &item);
        data[i].o.a.v = vq_retain(item.o.a.v);
        data[i].o.b.i = vCount(item.o.a.v);
        vCount(t) += data[i].o.b.i;
    }
    vOffs(t) = n;
    vq_release(views);
    return t;
}

vq_View ColCatVop (Vector views) {
    vq_View t, v;
    vq_Cell item;
    Vector metas;
    int i, j, c = 0, cols, n = vCount(views);
    vq_retain(views);
    assert(n > 0);
    metas = AllocDataVec(VQ_view, n);
    for (i = 0; i < n; ++i) {
        item.o.a.v = views;
        GetItem(i, &item);
        item.o.a.v = vMeta(item.o.a.v);
        vType(metas)->setter(metas, i, 0, &item);        
    }
    t = vq_new(vCount(views[0].o.a.v), RowCatVop(metas));
    for (i = 0; i < n; ++i) {
        item.o.a.v = views;
        GetItem(i, &item);
        v = item.o.a.v;
        cols = vCount(vMeta(v));
        for (j = 0; j < cols; ++j) {
            t[c++] = v[j];
            vq_retain(v[j].o.a.v);
        }
    }
    vq_release(views);
    return t;
}

#endif
