/*  View operators.
    $Id$
    This file is part of Vlerq, see src/vlerq.h for full copyright notice. */

static int o_plus (vqEnv env); /* forward */
    
static vqType ColMapGetter (int row, vqSlot *cp) {
    vqView v = cp->v;
    int c = cp->x.y.i, nc;
    vqSlot *aux = vwAuxP(v);
    vqSlot map = aux[1].v->col[0];
    if (map.v != 0) {
        vqType type = getcell(c, &map);
        if (type == VQ_int || type == VQ_pos)
            c = map.i;
    }
    v = aux[0].v;
    nc = vwCols(v);
    if (c >= nc && nc > 0)
        c %= nc;
    *cp = v->col[c];
    return getcell(row, cp);
}
static vqDispatch colmaptab = {
    "colmap", sizeof(struct vqView_s), 0, RowColMapCleaner, ColMapGetter
};
static vqView ColMapVop (vqView v, vqView map) {
    /* TODO: better - store col refs in a new view, avoids extra getter layer */
    vqView t, m = vwMeta(map), mm = RowMapVop(vwMeta(v), map);
    vqType type = ViewColType(map, 0);
    vqSlot *aux;
    t = IndirectView(&colmaptab, mm, vSize(v), 2 * sizeof(vqSlot));
    aux = vwAuxP(t);
    aux[0].v = vq_incref(v);
    if (vSize(m) > 0 && (type == VQ_int || type == VQ_pos))
        aux[1].v = vq_incref(map);
    vq_decref(vq_incref(map)); /* TODO: ugly, avoids mem leaks */
    return t;
}

static void CalcCleaner (vqColumn cp) {
    vqView v = cp->asview;
    lua_unref(vwEnv(v), (intptr_t) vwAuxP(v));
    ViewCleaner(cp);
}
static vqType CalcGetter (int row, vqSlot *cp) {
    vqType type;
    vqView v = cp->v;
    int col = cp->x.y.i;
    vqEnv env = vwEnv(cp->v);
    lua_rawgeti(env, LUA_REGISTRYINDEX, (intptr_t) vwAuxP(v));
    push_pos(env, row);
    push_pos(env, col);
    lua_call(env, 2, 1);
    if (lua_isnil(env, -1)) {
        lua_pop(env, 1);
        return VQ_nil;
    }
    type = check_cell(env, -1, VQ_TYPES[ViewColType(v, col)], cp);
    lua_pop(env, 1);
    return type;
}
static vqDispatch cvtab = {
    "calc", sizeof(struct vqView_s), 0, CalcCleaner, CalcGetter
};
static int o_calc (vqEnv env) {
    vqView v = check_view(env, 1), w = check_view(env, 2);
    v = IndirectView(&cvtab, w, vSize(v), 0);
    assert(lua_gettop(env) == 3);
    vwAuxP(v) = (void*) (intptr_t) luaL_ref(env, LUA_REGISTRYINDEX);
    return push_view(v);
}

static int o_pair (vqEnv env) {
    vqView v;
    int i, j, c = 0, nc, n = lua_gettop(env), rows = 0;
    assert(n > 0);
    
    lua_pushcfunction(env, o_plus);
    for (i = 0; i < n; ++i) {
        vqView w = vq_incref(check_view(env, i+1));
        push_view(vwMeta(w));
        if (i == 0)
            rows = vSize(w);
        else if (rows > vSize(w))
            rows = vSize(w);
        vq_decref(w);
    }
    lua_call(env, n, 1);
    
    v = alloc_view(&vtab, check_view(env, -1), rows, 0);
    
    for (i = 0; i < n; ++i) {
        vqView t = vq_incref(check_view(env, i+1));
        nc = vwCols(t);
        for (j= 0; j < nc; ++j) {
            v->col[c] = t->col[j];
            incColRef(v->col[c++].p);
        }
        vq_decref(t);
    }
    
    return push_view(v);
}

static void PlusCleaner (vqColumn cp) {
    vqSlot *aux;
    for (aux = vwAuxP(cp->asview); aux->v != 0; ++aux)
        vq_decref(aux->v);
    ViewCleaner(cp);
}
static vqType PlusGetter (int row, vqSlot *cp) {
    vqSlot *aux = vwAuxP(cp->v);
    while (row >= aux->x.y.i)
        row -= (aux++)->x.y.i;
    *cp = aux->v->col[cp->x.y.i];
    return getcell(row, cp);
}
static vqDispatch plustab = {
    "plus", sizeof(struct vqView_s), 0, PlusCleaner, PlusGetter
};
static int o_plus (vqEnv env) {
    vqView v;
    vqSlot *aux;
    int i, n = lua_gettop(env);
    assert(n > 0);
    v = IndirectView(&plustab, vwMeta(check_view(env, 1)), 0,
                                                    (n+1) * sizeof(vqSlot));
    /* TODO: could use vwMore, set aux to view count, and alloc one slot less */
    aux = vwAuxP(v);
    for (i = 0; i < n; ++i) {
        aux[i].v = vq_incref(check_view(env, i+1));
        aux[i].x.y.i = vSize(aux[i].v);
        vSize(v) += aux[i].x.y.i;
    }
    return push_view(v);
}

static vqType StepGetter (int row, vqSlot *cp) {
    int *aux = vwAuxP(cp->v);
    cp->i = aux[0] + aux[1] * (row / aux[2]);
    return VQ_pos;
}
static vqDispatch steptab = {
    "step", sizeof(struct vqView_s), 0, ViewCleaner, StepGetter
};
static int o_step (vqEnv env) {
    int rows = vSize(check_view(env, 1)), i = luaL_optinteger(env, 2, 1) - 1,
        j = luaL_optinteger(env, 3, 0), k = luaL_optinteger(env, 4, 0), *aux;
    vqView v;
    v = IndirectView(&steptab, desc2meta(env, ":P", 2), rows, 3 * sizeof(int));
    aux = vwAuxP(v);
    aux[0] = i; /* start */
    aux[1] = lua_isnoneornil(env, 3) ? 1 : j; /* step */
    aux[2] = k > 0 ? k : 1; /* rate */
    return push_view(v);
}

static vqView intvec2View (vqEnv env, int *v) {
    vqColumn cp = (void*) v;
    vqView w = alloc_view(&vtab, desc2meta(env, ":P", 2), vSize(cp), 0);
    w->col[0].p = v;
    /* TODO: awful cleanup hack if (vInfo(w->col[0].p).owner.col == 0) */
    own_datavec(w, 0);
    return w;
}

static vqView NewPosView (vqEnv env, int count) {
	return intvec2View(env, NewPosVec(count));
}

static int o_colmap (vqEnv env) {
    int c;
    vqView v = check_view(env, 1), w;
    if (lua_isnumber(env, 2))
        c = check_pos(env, 2);
    else if (lua_isstring(env, 2))
        c = colbyname(vwMeta(v), lua_tostring(env, 2));
    else
        return push_view(ColMapVop(v, check_view(env, 2)));
    if (c < 0)
        c += vwCols(v);
    w = NewPosView(env, 1);
    vq_setInt(w, 0, 0, c);
    return push_view(ColMapVop(v, w));
}

static int o_table (vqEnv env) {
    vqView v = check_view(env, 1);
    int c, r, nr = vSize(v);
    vqSlot cell;
    vqType t;
    
    lua_newtable(env);
    switch (vwCols(v)) {
        case 0:
            break;
        case 1:
            for (r = 0; r < nr; ++r) {
                cell = v->col[0];
                t = getcell(r, &cell);
                pushcell(env, VQ_TYPES[t], &cell);
                lua_rawseti(env, -2, r+1);
            }
            break;
        case 2:
            for (r = 0; r < nr; ++r) {
                for (c = 0; c < 2; ++c) {
                    cell = v->col[c];
                    t = getcell(r, &cell);
                    pushcell(env, VQ_TYPES[t], &cell);
                }
                lua_rawset(env, -3);
            }
            break;
        default:
            luaL_argerror(env, 1, "view has more than 2 columns");
    }
    return 1;
}

/* TODO: simplify, copy view is same as pair view with a single view arg */
static vqView copy_view (vqView v) {
    int c, nc = vwCols(v);
    vqView w = alloc_view(&vtab, vwMeta(v), vSize(v), 0);
    for (c = 0; c < nc; ++c) {
        w->col[c] = v->col[c];
        incColRef(w->col[c].p);
    }
    return w;
}

static int o_rename (vqEnv env) {
    vqView w, v = check_view(env, 1), m = vwMeta(v), mnew = copy_view(m);
    int c, r, nr = vSize(v);
    /* FIXME don't know if it's owned or shared? vq_decref(mnew->col[0].v); */
    mnew->col[0].p = new_datavec(VQ_string, nr);
    own_datavec(mnew, 0);
    for (r = 0; r < nr; ++r)
        vq_setString(mnew, r, 0, vq_getString(m, r, 0, ""));
    if (lua_isstring(env, 2))
        vq_setString(mnew, vSize(m) - 1, 0, lua_tostring(env, 2));
    else {
        if (lua_type(env, 2) != LUA_TTABLE)
            luaL_typerror(env, 1, "table");
        lua_pushnil(env);
        while (lua_next(env, 2)) {
            c = lua_isnumber(env, -2) ? check_pos(env, -2)
                                    : colbyname(m, luaL_checkstring(env, -2));
            if (c < 0)
                c += vSize(m);
            vq_setString(mnew, c, 0, luaL_checkstring(env, -1));
            lua_pop(env, 1);
        }
    }
    w = copy_view(v);
    vq_decref(vwMeta(w));
    vwMeta(w) = vq_incref(mnew);
    return push_view(w);
}

static vqView iota_view(vqEnv env, int n) {
    vqView v;
    /* TODO: avoid going through a Lua call for this */
    lua_getmetatable(env, 1);
    lua_getfield(env, -1, "step");
    lua_pushinteger(env, n);
    lua_call(env, 1, 1); /* vq(n):step(), i.e. iota identity mapping */
    v = check_view(env, -1);
    lua_pop(env, 1);
    return v;
}

static int o_omitmap (vqEnv env) {
    int r;
    vqView v = check_view(env, 1), w = check_view(env, 2);
    if (vSize(w) != 0) {
        /* TODO: if w is a 0-col view, return a step result of #w..#v-1 */
        /* TODO: generalize concept that 0-col view is iota when used as map */
        Buffer buffer;
        /* TODO: uses 1-char per entry for now, use bitmaps or ranges later */
        char* map = calloc(1, vSize(v));
        for (r = 0; r < vSize(w); ++r) {
            int i = vq_getInt(w, r, 0, -1);
            map[i] = 1;
        }
        InitBuffer(&buffer);
        for (r = 0; r < vSize(v); ++r)
            if (!map[r])
                ADD_INT_TO_BUF(buffer, r);
        free(map);
        return push_view(BufferAsPosView(env, &buffer));
    }
    return push_view(iota_view(env, vSize(v)));
}

static int o_replace (vqEnv env) {
    vqView v = check_view(env, 1), w = 0;
    int off = check_pos(env, 2), count = luaL_optinteger(env, 3, 1);
    if (!lua_isnoneornil(env, 4))
        w = check_view(env, 4);
    vq_replace(v, off, count, w);
    return push_view(v);
}

static const struct luaL_reg vqlib_ops[] = {
    {"calc", o_calc},
    {"colmap", o_colmap},
    {"omitmap", o_omitmap},
    {"pair", o_pair},
    {"plus", o_plus},
    {"rename", o_rename},
    {"replace", o_replace},
    {"step", o_step},
    {"table", o_table},
    {0, 0},
};
