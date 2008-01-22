/*  Implementation of sorting functions.
    $Id$
    This file is part of Vlerq, see src/vlerq.h for full copyright notice. */

static int ItemsEqual (vqType type, Item a, Item b) {
    switch (type) {

        case IT_int:
            return a.i == b.i;

        case IT_wide:
            return a.w == b.w;

        case IT_float:
            return a.f == b.f;

        case IT_double:
            return a.d == b.d;

        case IT_string:
            return strcmp(a.s, b.s) == 0;

        case IT_bytes:
            return a.u.len == b.u.len && memcmp(a.u.ptr, b.u.ptr, a.u.len) == 0;

        case IT_view:
            return ViewCompare(a.v, b.v) == 0;
            
        default: Assert(0); return -1;
    }
}

int RowEqual (vqView v1, int r1, vqView v2, int r2) {
    int c;
    vqType type;
    Item item1, item2;

    for (c = 0; c < ViewWidth(v1); ++c) {
        type = ViewColType(v1, c);
        item1 = GetViewItem(v1, r1, c, type);
        item2 = GetViewItem(v2, r2, c, type);

        if (!ItemsEqual(type, item1, item2))
            return 0;
    }

    return 1;
}

/* TODO: UTF-8 comparisons, also case-sensitive & insensitive */

static int ItemsCompare (vqType type, Item a, Item b, int lower) {
    switch (type) {

        case IT_int:
            return (a.i > b.i) - (a.i < b.i);

        case IT_wide:
            return (a.w > b.w) - (a.w < b.w);

        case IT_float:
            return (a.f > b.f) - (a.f < b.f);

        case IT_double:
            return (a.d > b.d) - (a.d < b.d);

        case IT_string:
            return (lower ? strcasecmp : strcmp)(a.s, b.s);

        case IT_bytes: {
            int f;

            if (a.u.len == b.u.len)
                return memcmp(a.u.ptr, b.u.ptr, a.u.len);

            f = memcmp(a.u.ptr, b.u.ptr, a.u.len < b.u.len ? a.u.len : b.u.len);
            return f != 0 ? f : a.u.len - b.u.len;
        }
        
        case IT_view:
            return ViewCompare(a.v, b.v);
            
        default: Assert(0); return -1;
    }
}

int RowCompare (vqView v1, int r1, vqView v2, int r2) {
    int c, f;
    vqType type;
    Item item1, item2;

    for (c = 0; c < ViewWidth(v1); ++c) {
        type = ViewColType(v1, c);
        item1 = GetViewItem(v1, r1, c, type);
        item2 = GetViewItem(v2, r2, c, type);

        f = ItemsCompare(type, item1, item2, 0);
        if (f != 0)
            return f < 0 ? -1 : 1;
    }

    return 0;
}

static int RowIsLess (vqView v, int a, int b) {
    int c, f;
    vqType type;
    Item va, vb;

    if (a != b)
        for (c = 0; c < ViewWidth(v); ++c) {
            type = ViewColType(v, c);
            va = GetViewItem(v, a, c, type);
            vb = GetViewItem(v, b, c, type);

            f = ItemsCompare(type, va, vb, 0);
            if (f != 0)
                return f < 0;
        }

    return a < b;
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

vqCell SortMap (vqView view) {
    int r, rows, *imap, *itmp;
    vqVeq seq;

    rows = ViewSize(view);

    if (rows <= 1 || ViewWidth(view) == 0)
        return NewIotaColumn(rows);

    seq = NewIntVec(rows, &imap);
    NewIntVec(rows, &itmp);
    
    for (r = 0; r < rows; ++r)
        imap[r] = itmp[r] = r;

    MergeSort(view, imap, rows, itmp);

    return SeqAsCol(seq);
}
