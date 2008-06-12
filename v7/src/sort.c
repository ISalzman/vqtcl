/*  Implementation of sorting functions.
    $Id$
    This file is part of Vlerq, see src/vlerq.h for full copyright notice. */

#include "strings.h"

static int ItemsEqual (vqType type, vqSlot a, vqSlot b) {
    switch (type) {
        case VQ_int:
        case VQ_pos:
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
    vqSlot item1, item2;

    for (c = 0; c < vwCols(v1); ++c) {
        type = ViewColType(v1, c);
        item1 = v1->col[c];
        item2 = v2->col[c];
        getcell(r1, &item1);
        getcell(r2, &item2);
        if (!ItemsEqual(type, item1, item2))
            return 0;
    }

    return 1;
}

/* TODO: UTF-8 comparisons, also case-sensitive & insensitive */

static int ItemsCompare (vqType type, vqSlot a, vqSlot b, int lower) {
    switch (type) {
        case VQ_int:
        case VQ_pos:
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
    vqSlot item1, item2;

    for (c = 0; c < vwCols(v1); ++c) {
        type = ViewColType(v1, c);
        item1 = v1->col[c];
        item2 = v2->col[c];
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
        
    rows1 = vSize(view1);
    rows2 = vSize(view2);
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
    int f = RowCompare(v, r1, v, r2);
    return f < 0 || (f == 0 && r1 < r2);
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

static vqView SortMap (vqView view) {
    int r, rows = vSize(view), *imap, *itmp;
    vqEnv env = vwEnv(view);
    vqView mvw = NewPosView(env, rows), tvw = vq_incref(NewPosView(env, rows));
    assert(rows > 1 && vwCols(view) > 0);
    imap = mvw->col[0].p;
    itmp = tvw->col[0].p;
    for (r = 0; r < rows; ++r)
        imap[r] = itmp[r] = r;
    MergeSort(view, imap, rows, itmp);
    vq_decref(tvw);
    return mvw;
}

static int s_sortmap (vqEnv env) {
    vqView v = check_view(env, 1);
    if (vSize(v) <= 1 || vwCols(v) == 0)
        return push_view(iota_view(vwEnv(v), vSize(v)));
    return push_view(SortMap(v));
}

#define MAX_KEYS 10

typedef struct SelectCriteria_s {
    int numkeys;
    int keys[MAX_KEYS];
    vqSlot vals[MAX_KEYS];
} SelectCriteria;

static int select_fun (vqEnv env) {
    int k;
    vqSlot *cp = checkrow(env, 1);
    SelectCriteria *scp = lua_touserdata(env, lua_upvalueindex(1));
    assert(scp != 0);
    for (k = 0; k < scp->numkeys; ++k) {
        vqSlot cell = cp->v->col[scp->keys[k]];
        vqType t = getcell(cp->x.y.i, &cell);
        if (t == VQ_nil || !ItemsEqual(t, cell, scp->vals[k]))
            return 0;
    }
    lua_pushboolean(env, 1);
    return 1;
}

static int s_selectmap (vqEnv env) {
    int k;
    SelectCriteria sc;
    vqView v = check_view(env, 1);
    if (lua_istable(env, 2)) {
        sc.numkeys = 0;
        lua_pushnil(env);
        while (lua_next(env, 2)) {
            /* TODO: numeric column indices, also < 0 */
            const char *s = luaL_checkstring(env, -2);
            int c = colbyname(vwMeta(v), s);
            vqType t = ViewColType(v, c);
            sc.keys[sc.numkeys] = c;
            check_cell(env, -1, VQ_TYPES[t], sc.vals+sc.numkeys);
            lua_pop(env, 1);
            ++sc.numkeys;
        }
    } else {
        sc.numkeys = lua_gettop(env) - 1;
        for (k = 0; k < sc.numkeys; ++k) {
            vqType t = ViewColType(v, k);
            sc.keys[k] = k;
            check_cell(env, 2+k, VQ_TYPES[t], sc.vals+k);
        }
    }
    if (sc.numkeys == 0)
        return push_view(iota_view(env, vSize(v)));
    lua_getmetatable(env, 1);
    lua_getfield(env, -1, "wheremap");
    lua_pushvalue(env, 1);
    lua_pushlightuserdata(env, &sc);
    lua_pushcclosure(env, select_fun, 1);
    lua_call(env, 2, 1); /* v:wheremap(f) */
    return 1;
}

static int s_wheremap (vqEnv env) {
    vqSlot *cp;
    struct Buffer buffer;
    vqView v = check_view(env, 1);
    int nr = vSize(v);
    luaL_checktype(env, 2, LUA_TFUNCTION);
    cp = tagged_udata(env, sizeof *cp, "vq.row");
    cp->v = vq_incref(v);
    InitBuffer(&buffer);
    for (cp->x.y.i = 0; cp->x.y.i < nr; ++(cp->x.y.i)) {
        lua_pushvalue(env, 2);
        lua_pushvalue(env, 3);
        lua_call(env, 1, 1); /* f(row) */
        if (lua_toboolean(env, -1))
            ADD_INT_TO_BUF(buffer, cp->x.y.i);
        lua_pop(env, 1);
    }
    return push_view(BufferAsPosView(env, &buffer));
}

static const struct luaL_reg vqlib_sort[] = {
    {"selectmap", s_selectmap},
    {"sortmap", s_sortmap},
    {"wheremap", s_wheremap},
    {0, 0},
};

static int row_eq (vqEnv env) {
    vqSlot *r1 = checkrow(env, 1), *r2 = checkrow(env, 2);
    lua_pushboolean(env, RowEqual(r1->v, r1->x.y.i, r2->v, r2->x.y.i));
    return 1;
}

static int row_lt (vqEnv env) {
    vqSlot *r1 = checkrow(env, 1), *r2 = checkrow(env, 2);
    lua_pushboolean(env, RowCompare(r1->v, r1->x.y.i, r2->v, r2->x.y.i) < 0);
    return 1;
}

static const struct luaL_reg vqlib_row_cmp_m[] = {
    {"__eq", row_eq},
    {"__lt", row_lt},
    {0, 0},
};

static int view_eq (vqEnv env) {
    vqView v1 = check_view(env, 1), v2 = check_view(env, 2);
    lua_pushboolean(env, ViewCompare(v1, v2) == 0);
    return 1;
}

static int view_lt (vqEnv env) {
    vqView v1 = check_view(env, 1), v2 = check_view(env, 2);
    lua_pushboolean(env, ViewCompare(v1, v2) < 0);
    return 1;
}

static const struct luaL_reg vqlib_view_cmp_m[] = {
    {"__eq", view_eq},
    {"__lt", view_lt},
    {0, 0},
};
