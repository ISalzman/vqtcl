/*  Implementation of sorting functions.
    $Id$
    This file is part of Vlerq, see src/vlerq.h for full copyright notice. */

static int ItemsEqual (vqType type, vqCell a, vqCell b) {
    switch (type) {
        case VQ_int:
            return a.i == b.i;
        case VQ_long:
            return a.l == b.l;
        case VQ_float:
            return a.f == b.f;
        case VQ_double:
            return a.d == b.d;
        case VQ_string:
            return strcmp(a.s, b.s) == 0;
        case VQ_bytes:
            return a.x.y.i == b.x.y.i && memcmp(a.p, b.p, a.x.y.i) == 0;
        case VQ_view:
            return ViewCompare(a.v, b.v) == 0;
        default: assert(0); return -1;
    }
}

int RowEqual (vqView v1, int r1, vqView v2, int r2) {
    int c;
    vqType type;
    vqCell item1, item2;

    for (c = 0; c < vwCols(v1); ++c) {
        type = ViewColType(v1, c);
        item1 = vwCol(v1,c);
        item2 = vwCol(v2,c);
        getcell(r1, &item1);
        getcell(r2, &item2);
        if (!ItemsEqual(type, item1, item2))
            return 0;
    }

    return 1;
}

/* TODO: UTF-8 comparisons, also case-sensitive & insensitive */

static int ItemsCompare (vqType type, vqCell a, vqCell b, int lower) {
    switch (type) {
        case VQ_int:
            return (a.i > b.i) - (a.i < b.i);
        case VQ_long:
            return (a.l > b.l) - (a.l < b.l);
        case VQ_float:
            return (a.f > b.f) - (a.f < b.f);
        case VQ_double:
            return (a.d > b.d) - (a.d < b.d);
        case VQ_string:
            return (lower ? strcasecmp : strcmp)(a.s, b.s);
        case VQ_bytes: {
            int f;
            if (a.x.y.i == b.x.y.i)
                return memcmp(a.p, b.p, a.x.y.i);
            f = memcmp(a.p, b.p, a.x.y.i < b.x.y.i ? a.x.y.i : b.x.y.i);
            return f != 0 ? f : a.x.y.i - b.x.y.i;
        }
        case VQ_view:
            return ViewCompare(a.v, b.v);
        default: assert(0); return -1;
    }
}

int RowCompare (vqView v1, int r1, vqView v2, int r2) {
    int c, f;
    vqType type;
    vqCell item1, item2;

    for (c = 0; c < vwCols(v1); ++c) {
        type = ViewColType(v1, c);
        item1 = vwCol(v1,c);
        item2 = vwCol(v2,c);
        getcell(r1, &item1);
        getcell(r2, &item2);
        f = ItemsCompare(type, item1, item2, 0);
        if (f != 0)
            return f < 0 ? -1 : 1;
    }

    return 0;
}

int ViewCompare (vqView view1, vqView view2) {
    int f, r, rows1, rows2;
    
    if (view1 == view2)
        return 0;
    f = ViewCompare(vwMeta(view1), vwMeta(view2));
    if (f != 0)
        return f;
        
    rows1 = vwRows(view1);
    rows2 = vwRows(view2);
    for (r = 0; r < rows1; ++r) {
        if (r >= rows2)
            return 1;
        f = RowCompare(view1, r, view2, r);
        if (f != 0)
            return f < 0 ? -1 : 1;
    }
    return rows1 < rows2 ? -1 : 0;
}

static int RowIsLess (vqView v, int r1, int r2) {
    return r1 < r2 || RowCompare(v, r1, v, r2) < 0;
}

static int TestAndSwap (vqView v, int *a, int *b) {
    if (RowIsLess(v, *b, *a)) {
        int t = *a;
        *a = *b;
        *b = t;
        return 1;
    }
    return 0;
}

static void MergeSort (vqView v, int *ar, int nr, int *scr) {
    switch (nr) {
        case 2:
            TestAndSwap(v, ar, ar+1);
            break;
        case 3:
            TestAndSwap(v, ar, ar+1);
            if (TestAndSwap(v, ar+1, ar+2))
                TestAndSwap(v, ar, ar+1);
            break;
        case 4: /* TODO: optimize with if's */
            TestAndSwap(v, ar, ar+1);
            TestAndSwap(v, ar+2, ar+3);
            TestAndSwap(v, ar, ar+2);
            TestAndSwap(v, ar+1, ar+3);
            TestAndSwap(v, ar+1, ar+2);
            break;
            /* TODO: also special-case 5-item sort? */
        default: {
            int s1 = nr / 2, s2 = nr - s1;
            int *f1 = scr, *f2 = scr + s1, *t1 = f1 + s1, *t2 = f2 + s2;
            MergeSort(v, f1, s1, ar);
            MergeSort(v, f2, s2, ar+s1);
            for (;;)
                if (RowIsLess(v, *f1, *f2)) {
                    *ar++ = *f1++;
                    if (f1 >= t1) {
                        while (f2 < t2)
                            *ar++ = *f2++;
                        break;
                    }
                } else {
                    *ar++ = *f2++;
                    if (f2 >= t2) {
                        while (f1 < t1)
                            *ar++ = *f1++;
                        break;
                    }
                }
        }
    }
}

static vqVec NewIntVec (int count, int **dataptr) {
	vqVec seq = new_datavec(VQ_int, count);
	if (dataptr != 0)
		*dataptr = (int*) seq;
	return seq;
}

static vqVec SortMap (vqView view) {
    vqVec seq, tv;
    int r, rows = vwRows(view), *imap, *itmp;
    if (rows <= 1 || vwCols(view) == 0)
        return 0;
    seq = NewIntVec(rows, &imap);
    tv = vq_incref(NewIntVec(rows, &itmp));
    for (r = 0; r < rows; ++r)
        imap[r] = itmp[r] = r;
    MergeSort(view, imap, rows, itmp);
    vq_decref(tv);
    return seq;
}

static int s_sortmap (lua_State *L) {
    vqView v = checkview(L, 1), w;
    vqVec map = SortMap(v);
    assert(map != 0);
    w = vq_new(desc2meta(L, ":I", 2), vSize(map));
    vwCol(w,0).v = vq_incref(map);
    return push_view(w);
}

static const struct luaL_reg lvqlib_sort[] = {
    {"sortmap", s_sortmap},
    {0, 0},
};
