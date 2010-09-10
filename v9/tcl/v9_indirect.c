/* V9 indirect views
   2009-05-08 <jcw@equU4.com> http://opensource.org/licenses/mit-license.php
   $Id$ */
 
#include "v9.priv.h"

typedef struct StepViewInfo {
    int start;
    int step;
    int rate;
} StepViewInfo;
    
static void StepViewCleaner (Vector vp) {
	V9View v = (V9View) vp;
	V9_Release(v->meta);
}

static V9Types StepViewGetter (int row, V9Item* pitem) {
    StepViewInfo* vip = pitem->v->extra;
    pitem->i = vip->start + vip->step * (row / vip->rate);
    return V9T_int;
}

static struct VectorType vtStepView = {
    "step", -1, StepViewCleaner, StepViewGetter
};

V9View V9_StepView (int count, int start, int step, int rate) {
    V9View v = IndirectView(V9_DescAsMeta(":I", 0), count, &vtStepView,
                                                        sizeof (StepViewInfo));
    StepViewInfo* vip = v->extra;
    vip->start = start;
    vip->step = step;
    vip->rate = rate;
    
    return v;
}

typedef struct RowMapViewInfo {
    V9View data;
    V9View map;
} RowMapViewInfo;
    
static void RowMapViewCleaner (Vector vp) {
	V9View v = (V9View) vp;
	V9_Release(v->meta);
    RowMapViewInfo* vip = v->extra;
	V9_Release(vip->data);
	V9_Release(vip->map);
}

static V9Types RowMapViewGetter (int row, V9Item* pitem) {
    RowMapViewInfo* vip = pitem->v->extra;
    int r = V9_GetInt(vip->map, row, 0);
    if (r < 0) // special cased to also support "cluster" indexing
        r = V9_GetInt(vip->map, row + r, 0);
    *pitem = vip->data->col[pitem->c.i];
    return GetItem(r, pitem);
}

static struct VectorType vtRowMapView = {
    "rowmap", -1, RowMapViewCleaner, RowMapViewGetter
};

V9View V9_RowMapView (V9View data, V9View map) {
    V9View v = IndirectView(data->meta, Head(map).count, &vtRowMapView,
                                                    sizeof (RowMapViewInfo));
    RowMapViewInfo* vip = v->extra;
    vip->data = V9_AddRef(data);
    vip->map = V9_AddRef(map);
    
    return v;
}

// not strictly an indirect view, but based on rowmap
V9View V9_ColMapView (V9View data, V9View map) {
    V9View meta = V9_RowMapView(data->meta, map);
    V9View v = V9_NewDataView(meta, 0);
    Head(v).count = Head(data).count;
    Head(v).type = Head(data).type;
    
    int c, ncols = Head(meta).count;
    for (c = 0; c < ncols; ++c) {
        v->col[c] = data->col[V9_GetInt(map, c, 0)];
        IncRef(v->col[c].c.p);
    }
    
    return v;
}

typedef struct PlusViewInfo {
    V9View one;
    V9View two;
} PlusViewInfo;
    
static void PlusViewCleaner (Vector vp) {
	V9View v = (V9View) vp;
	V9_Release(v->meta);
    PlusViewInfo* vip = v->extra;
	V9_Release(vip->one);
	V9_Release(vip->two);
}

static V9Types PlusViewGetter (int row, V9Item* pitem) {
    PlusViewInfo* vip = pitem->v->extra;
    V9View v = vip->one;
    if (row >= Head(v).count) {
        row -= Head(v).count;
        v = vip->two;
    }
    int col = pitem->c.i;
    *pitem = V9_Get(v, row, col, 0);
    return V9_GetInt(v->meta, col, 1);
}

static struct VectorType vtPlusView = {
    "plus", -1, PlusViewCleaner, PlusViewGetter
};

V9View V9_PlusView (V9View one, V9View two) {
    V9View v = IndirectView(one->meta, Head(one).count + Head(two).count,
                                        &vtPlusView, sizeof (PlusViewInfo));
    PlusViewInfo* vip = v->extra;
    vip->one = V9_AddRef(one);
    vip->two = V9_AddRef(two);
    
    return v;
}

#if 1
typedef struct PairViewInfo {
    V9View one;
    V9View two;
} PairViewInfo;
    
static void PairViewCleaner (Vector vp) {
	V9View v = (V9View) vp;
	V9_Release(v->meta);
    PairViewInfo* vip = v->extra;
	V9_Release(vip->one);
	V9_Release(vip->two);
}

static V9Types PairViewGetter (int row, V9Item* pitem) {
    PairViewInfo* vip = pitem->v->extra;
    V9View v = vip->one;
    int col = pitem->c.i;
    if (col >= Head(v->meta).count) {
        col -= Head(v->meta).count;
        v = vip->two;
    }
    *pitem = V9_Get(v, row, col, 0);
    return V9_GetInt(v->meta, col, 1);
}

static struct VectorType vtPairView = {
    "pair", -1, PairViewCleaner, PairViewGetter
};

// not strictly an indirect view, but based on plus
V9View V9_PairView (V9View one, V9View two) {
    int rows = Head(one).count;
    if (rows > Head(two).count)
        rows = Head(two).count;
        
    V9View v = IndirectView(V9_PlusView(one->meta, two->meta), rows,
                                        &vtPairView, sizeof (PairViewInfo));
    PairViewInfo* vip = v->extra;
    vip->one = V9_AddRef(one);
    vip->two = V9_AddRef(two);

    return v;
}
#endif

#if 0
static struct VectorType vtPairView = {
    "pair", -1
};

// not strictly an indirect view, but based on plus
V9View V9_PairView (V9View one, V9View two) {
    int rows = Head(one).count;
    if (rows > Head(two).count)
        rows = Head(two).count;

    V9View meta = V9_PlusView(one->meta, two->meta);
    // V9View v = IndirectView(meta, rows, &vtPairView, 0);
    V9View v = V9_NewDataView(meta, 0);
    Head(v).count = rows;
    // Head(v).type = Head(one).type; //XXX what if two's type differs?
    Head(v).type = &vtPairView;
    
    int c1, c2, ncols1 = Head(one).count, ncols2 = Head(two).count;
    for (c1 = 0; c1 < ncols1; ++c1) {
        v->col[c1] = one->col[c1];
        IncRef(v->col[c1].c.p);
    }
    for (c2 = 0; c2 < ncols2; ++c2) {
        v->col[c1+c2] = two->col[c2];
        IncRef(v->col[c1+c2].c.p);
    }
    
    return v;
}
#endif

static void TimesViewCleaner (Vector vp) {
	V9View v = (V9View) vp;
	V9_Release(v->meta);
    V9View data = v->extra;
	V9_Release(data);
}

static V9Types TimesViewGetter (int row, V9Item* pitem) {
    V9View v = pitem->v->extra;
    if (Head(v).count != 0)
        row %= Head(v).count;
    *pitem = v->col[pitem->c.i];
    return GetItem(row, pitem);
}

static struct VectorType vtTimesView = {
    "times", -1, TimesViewCleaner, TimesViewGetter
};

V9View V9_TimesView (V9View data, int n) {
    V9View v = IndirectView(data->meta, n * Head(data).count, &vtTimesView, 0);
    v->extra = V9_AddRef(data);
    return v;
}
