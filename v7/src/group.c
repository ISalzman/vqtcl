/*  Grouping and ungrouping.
    $Id$
    This file is part of Vlerq, see src/vlerq.h for full copyright notice. */

static vqView MakeMetaSubview (vqView view) {
    vqView result;
    result = vq_new(vwMeta(empty_meta(vwEnv(view))), 1);
    vq_setMetaRow(result, 0, "", VQ_view, vwMeta(view));
    return result;
}

#define GV_parent(a)    ((vqSlot*) vwAuxP(a))[0].v
#define GV_start(a)     ((vqSlot*) vwAuxP(a))[0].x.y.v
#define GV_group(a)     ((vqSlot*) vwAuxP(a))[1].v
#define GV_cache(a)     ((vqSlot*) vwAuxP(a))[1].x.y.c

static void GroupedCleaner (vqColumn cp) {
    vqView v = cp->asview;
    vq_decref(GV_parent(v));
    vq_decref(GV_start(v));
    vq_decref(GV_group(v));
    release_vec(GV_cache(v));
    ViewCleaner(cp);
}

static vqType GroupedGetter (int row, vqSlot *cp) {
    vqView seq = cp->p;
    vqView *subviews = (void*) GV_cache(seq);
    
    if (subviews[row] == 0) {
        vqView start = GV_start(seq), map;
        vqEnv env = vwEnv(start);
        int spos = row > 0 ? vq_getInt(start, row-1, 0, -1) : 0;
        int next = vq_getInt(start, row, 0, -1);
        
        /* TODO: avoid going through a Lua call for this */
        luaL_getmetatable(env, "vq.view");                /* mt */
        lua_getfield(env, -1, "step");                    /* mt f */
        push_view(vq_new(empty_meta(env), next - spos));  /* mt f v */
        lua_pushinteger(env, spos+1);                     /* mt f v s */
        lua_call(env, 2, 1); /* vq(spos+1):step(next-spos) */
        map = check_view(env, -1);                        /* mt v */

        /* TODO: clumsy, goes through two row maps, first is just a slice */
        map = RowMapVop(GV_group(seq), map);
        subviews[row] = vq_incref(RowMapVop(GV_parent(seq), map));

        lua_pop(env, 2);                      /* <> */
    }
    
    cp->v = subviews[row];
    return VQ_view;
}

static vqDispatch grtab = {
    "grouped", sizeof(struct vqView_s), 0, GroupedCleaner, GroupedGetter
};

static vqView GroupedView (vqView view, vqView startcol, vqView groupcol) {
    int groups = vSize(startcol);
    vqView v = IndirectView(&grtab, MakeMetaSubview(view), 
                                        groups, 2 * sizeof(vqSlot));
    assert(vwCols(startcol) == 1);
    assert(vwCols(groupcol) == 1);
    GV_parent(v) = vq_incref(view);
    GV_start(v) = vq_incref(startcol);
    GV_group(v) = vq_incref(groupcol);
    GV_cache(v) = new_datavec(VQ_view, groups);
    
    return v;
}

static int g_grouped (vqEnv env) {
    vqView v = check_view(env, 1), sc = check_view(env, 2), gc = check_view(env, 3);
    return push_view(GroupedView(v, sc, gc));
}

/* TODO: ungrouper code is very similar to rowmap, unify where possible */

static void UngrouperCleaner (vqColumn cp) {
    vqView v = cp->asview;
    vqSlot *aux = vwAuxP(v);
    vq_decref(aux[0].v);
    decColRef(aux[1].p);
    ViewCleaner(cp);
}

static vqType UngrouperGetter (int row, vqSlot *cp) {
    int prow;
    vqView v = cp->v;
    vqSlot *aux = vwAuxP(v);
    vqSlot map = aux[1];
    getcell(row, &map);
    prow = map.i;
    
    if (prow < 0) {
        vqSlot map2 = aux[1];
        getcell(row + prow, &map2);
        prow = map2.i;
        row = - map.i;
    } else
        row = 0;
    v = vq_getView(aux[0].v, prow, 0, 0);
    *cp = v->col[cp->x.y.i];
    return getcell(row, cp);
}

static vqDispatch ugtab = { 
    "ungrouper", sizeof(struct vqView_s), 0, UngrouperCleaner, UngrouperGetter
};

static vqView Ungrouper (vqView v, vqView map) {
    vqView t, m = vwMeta(v);
    vqSlot *aux;
    m = vq_getView(m, 0, 2, 0);
    t = IndirectView(&ugtab, m, vSize(map), 2 * sizeof(vqSlot));
    aux = vwAuxP(t);
    aux[0].v = vq_incref(v);
    aux[1] = map->col[0];
    incColRef(aux[1].p);
    return t;
}

static int g_ungrouper (vqEnv env) {
    vqView v = check_view(env, 1), w = check_view(env, 2);
    return push_view(Ungrouper(v, w));
}

static vqView UngroupMap (vqView v) {
    int r;
    struct Buffer buffer;
    
    InitBuffer(&buffer);

    for (r = 0; r < vSize(v); ++r) {
        vqView subv = vq_getView(v, r, 0, 0);
        int i, n = vSize(subv);
        if (n > 0) {
            ADD_INT_TO_BUF(buffer, r);
            for (i = 1; i < n; ++i)
                ADD_INT_TO_BUF(buffer, -i);
        }
    }

    return BufferAsPosView(vwEnv(v), &buffer);
}

static int g_ungroupmap (vqEnv env) {
    vqView v = check_view(env, 1);
    return push_view(UngroupMap(v));
}

static void BlockedCleaner (vqColumn cp) {
    vqView v = cp->asview;
    vqSlot *sp = vwAuxP(v);
    vq_decref(sp->v);
    vq_decref(sp->x.y.v);
    ViewCleaner(cp);
}

static vqType BlockedGetter (int row, vqSlot *cp) {
    int block, col = cp->x.y.i;
    vqSlot *aux = vwAuxP(cp->v);
    vqView v, parent = aux->v;
    const int* data = aux->x.y.v->col[0].p;

    for (block = 0; block + data[block] < row; ++block)
        ;

    if (row == block + data[block]) {
        row = block;
        block = vSize(parent) - 1;
    } else if (block > 0)
        row -= block + data[block-1];

    v = vq_getView(parent, block, 0, 0);
    *cp = v->col[col];   
    return getcell(row, cp);
}

static vqDispatch blockedvt = {
    "blocked", sizeof(struct vqView_s), 0, BlockedCleaner, BlockedGetter
};

static vqView BlockedView (vqView v) {
    int r, rows = vSize(v), *limits, tally = 0;
    vqView m, w, t;
    vqSlot *aux;

    /* view must have exactly one subview column */
    assert(vwCols(v) == 1);
    
    t = NewPosView(vwEnv(v), rows);
    limits = t->col[0].p;
    for (r = 0; r < rows; ++r) {
        tally += vSize(vq_getView(v, r, 0, 0));
        limits[r] = tally;
    }
    
    m = vq_getView(vwMeta(v), 0, 2, 0);
    w = IndirectView(&blockedvt, m, tally, sizeof(vqSlot));
    aux = vwAuxP(w);
    aux->v = vq_incref(v);
    aux->x.y.v = vq_incref(t);
    
    return w;
}

static int g_blocked (vqEnv env) {
    vqView v = check_view(env, 1);
    return push_view(BlockedView(v));
}

static const struct luaL_reg vqlib_group[] = {
    {"blocked", g_blocked},
    {"grouped", g_grouped},
    {"ungrouper", g_ungrouper},
    {"ungroupmap", g_ungroupmap},
    {0, 0},
};
