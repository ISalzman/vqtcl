/* V9 sorting and comparing
   2009-05-08 <jcw@equU4.com> http://opensource.org/licenses/mit-license.php
   $Id$ */
 
#include "v9.priv.h"
#include "string.h"

static int ItemsEqual (V9Types type, V9Item a, V9Item b) {
    switch (type) {
        case V9T_int:
            return a.i == b.i;
        case V9T_long:
            return a.l == b.l;
        case V9T_float:
            return a.f == b.f;
        case V9T_double:
            return a.d == b.d;
        case V9T_string:
            return strcmp(a.s, b.s) == 0;
        case V9T_bytes:
            return a.c.i == b.c.i && memcmp(a.p, b.p, a.c.i) == 0;
        case V9T_view:
            return V9_ViewCompare(a.v, b.v) == 0;
        default: assert(0); return 0;
    }
}

int V9_RowEqual (V9View v1, int r1, V9View v2, int r2) {
    int c, ncols = Head(v1->meta).count;
    for (c = 0; c < ncols; ++c) {
        V9Types type = V9_GetInt(v1->meta, c, 1);
        V9Item item1 = v1->col[c];
        V9Item item2 = v2->col[c];
        GetItem(r1, &item1);
        GetItem(r2, &item2);
        if (!ItemsEqual(type, item1, item2))
            return 0;
    }

    return 1;
}

/* TODO: UTF-8 comparisons, also case-sensitive & insensitive */

static int ItemsCompare (V9Types type, V9Item a, V9Item b, int lower) {
    switch (type) {
        case V9T_int:
            return (a.i > b.i) - (a.i < b.i);
        case V9T_long:
            return (a.l > b.l) - (a.l < b.l);
        case V9T_float:
            return (a.f > b.f) - (a.f < b.f);
        case V9T_double:
            return (a.d > b.d) - (a.d < b.d);
        case V9T_string:
            return (lower ? strcasecmp : strcmp)(a.s, b.s);
        case V9T_bytes: {
            int f;
            if (a.c.i == b.c.i)
                return memcmp(a.p, b.p, a.c.i);
            f = memcmp(a.p, b.p, a.c.i < b.c.i ? a.c.i : b.c.i);
            return f != 0 ? f : a.c.i - b.c.i;
        }
        case V9T_view:
            return V9_ViewCompare(a.v, b.v);
        default: assert(0); return -1;
    }
}

int V9_RowCompare (V9View v1, int r1, V9View v2, int r2) {
    int c, f;
    V9Types type;
    V9Item item1, item2;

    for (c = 0; c < Head(v1->meta).count; ++c) {
        type = V9_GetInt(v1->meta, c, 1);
        item1 = v1->col[c];
        item2 = v2->col[c];
        GetItem(r1, &item1);
        GetItem(r2, &item2);
        f = ItemsCompare(type, item1, item2, 0);
        if (f != 0)
            return f < 0 ? -1 : 1;
    }

    return 0;
}

int V9_ViewCompare (V9View view1, V9View view2) {
    int f, r, rows1, rows2;
    
    if (view1 == view2)
        return 0;
    f = V9_ViewCompare(view1->meta, view2->meta);
    if (f != 0)
        return f;
        
    rows1 = Head(view1).count;
    rows2 = Head(view2).count;
    for (r = 0; r < rows1; ++r) {
        if (r >= rows2)
            return 1;
        f = V9_RowCompare(view1, r, view2, r);
        if (f != 0)
            return f < 0 ? -1 : 1;
    }
    return rows1 < rows2 ? -1 : 0;
}

static int RowIsLess (V9View v, int r1, int r2) {
    int f = V9_RowCompare(v, r1, v, r2);
    return f < 0 || (f == 0 && r1 < r2);
}

static int TestAndSwap (V9View v, int *a, int *b) {
    if (RowIsLess(v, *b, *a)) {
        int t = *a;
        *a = *b;
        *b = t;
        return 1;
    }
    return 0;
}

static void MergeSort (V9View v, int *ar, int nr, int *scr) {
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

V9View V9_SortMap (V9View view) {
    assert(Head(view->meta).count > 0);
    int rows = Head(view).count;
    V9View mvw = V9_NewDataView(V9_DescAsMeta(":I", 0), rows);
    if (rows > 1) {
        V9View tvw = V9_NewDataView(V9_DescAsMeta(":I", 0), rows);
        int* imap = mvw->col[0].p;
        int* itmp = tvw->col[0].p;
        int r;
        for (r = 0; r < rows; ++r)
            imap[r] = itmp[r] = r;
        MergeSort(view, imap, rows, itmp);
        V9_Release(tvw);
    }
    return mvw;
}

#if 0
#define MAX_KEYS 10

typedef struct SelectCriteria_s {
    int numkeys;
    int keys[MAX_KEYS];
    V9Item vals[MAX_KEYS];
} SelectCriteria;

static int select_fun (vqEnv env) {
    int k;
    V9Item *cp = checkrow(env, 1);
    SelectCriteria *scp = lua_touserdata(env, lua_upvalueindex(1));
    assert(scp != 0);
    for (k = 0; k < scp->numkeys; ++k) {
        V9Item cell = cp->v->col[scp->keys[k]];
        V9Types t = GetItem(cp->c.i, &cell);
        if (t == V9T_nil || !ItemsEqual(t, cell, scp->vals[k]))
            return 0;
    }
    lua_pushboolean(env, 1);
    return 1;
}

static int s_selectmap (vqEnv env) {
    int k;
    SelectCriteria sc;
    V9View v = check_view(env, 1);
    if (lua_istable(env, 2)) {
        sc.numkeys = 0;
        lua_pushnil(env);
        while (lua_next(env, 2)) {
            /* TODO: numeric column indices, also < 0 */
            const char *s = luaL_checkstring(env, -2);
            int c = colbyname(v->meta, s);
            V9Types t = V9_GetInt(v->meta, c, 1);
            sc.keys[sc.numkeys] = c;
            check_cell(env, -1, V9_TYPEMAP[t], sc.vals+sc.numkeys);
            lua_pop(env, 1);
            ++sc.numkeys;
        }
    } else {
        sc.numkeys = lua_gettop(env) - 1;
        for (k = 0; k < sc.numkeys; ++k) {
            V9Types t = V9_GetInt(v->meta, k, 1);
            sc.keys[k] = k;
            check_cell(env, 2+k, V9_TYPEMAP[t], sc.vals+k);
        }
    }
    if (sc.numkeys == 0)
        return push_view(iota_view(env, Head(v).count));
    lua_getmetatable(env, 1);
    lua_getfield(env, -1, "wheremap");
    lua_pushvalue(env, 1);
    lua_pushlightuserdata(env, &sc);
    lua_pushcclosure(env, select_fun, 1);
    lua_call(env, 2, 1); /* v:wheremap(f) */
    return 1;
}

static int s_wheremap (vqEnv env) {
    V9Item *cp;
    struct Buffer buffer;
    V9View v = check_view(env, 1);
    int nr = Head(v).count;
    luaL_checktype(env, 2, LUA_TFUNCTION);
    cp = tagged_udata(env, sizeof *cp, "vq.row");
    cp->v = vq_incref(v);
    InitBuffer(&buffer);
    for (cp->c.i = 0; cp->c.i < nr; ++(cp->c.i)) {
        lua_pushvalue(env, 2);
        lua_pushvalue(env, 3);
        lua_call(env, 1, 1); /* f(row) */
        if (lua_toboolean(env, -1))
            ADD_INT_TO_BUF(buffer, cp->c.i);
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
#endif
