/*  View operators.
    $Id$
    This file is part of Vlerq, see lvq/vlerq.h for full copyright notice. */

static void CallCleaner (void *p) {
    vqView v = p;
    lua_unref(vwState(v), vwAuxI(v));
    IndirectCleaner(p);
}
static vqType CallGetter (int row, vqCell *cp) {
    vqType type;
    vqView v = cp->v;
    int col = cp->x.y.i;
    lua_State *L = vwState(cp->v);
    lua_rawgeti(L, LUA_REGISTRYINDEX, vwAuxI(v));
    lua_pushinteger(L, row);
    lua_pushinteger(L, col);
    lua_call(L, 2, 1);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return VQ_nil;
    }
    type = vq_getInt(vwMeta(v), col, 1, VQ_nil) & VQ_TYPEMASK;
    type = check_cell(L, -1, VQ_TYPES[type], cp);
    lua_pop(L, 1);
    return type;
}
static vqDispatch cvtab = {
    "call", sizeof(struct vqView_s), 0, CallCleaner, CallGetter
};
static int vops_call (lua_State *L) {
    vqView v;
    LVQ_ARGS(L,A,"VVN");
    v = IndirectView(&cvtab, A[1].v, vwRows(A[0].v), 0);
    assert(lua_gettop(L) == 3);
    vwAuxP(v) = (void*) luaL_ref(L, LUA_REGISTRYINDEX);
    return push_view(v);
}

static int vops_meta (lua_State *L) {
    LVQ_ARGS(L,A,"V");
    return push_view(vwMeta(A[0].v));
}

static vqType StepGetter (int row, vqCell *cp) {
    int *aux = vwAuxP(cp->v);
    cp->i = aux[0] + aux[1] * (row / aux[2]);
    return VQ_int;
}
static vqDispatch steptab = {
    "step", sizeof(struct vqView_s), 0, IndirectCleaner, StepGetter
};
static int vops_step (lua_State *L) {
    int rows, *aux;
    vqView v;
    LVQ_ARGS(L,A,"Viii");
    rows = vwRows(A[0].v);
    v = IndirectView(&steptab, desc2meta(L, ":I", 2), rows, 3 * sizeof(int));
    aux = vwAuxP(v);
    aux[0] = A[1].i; /* start */
    aux[1] = lua_isnoneornil(L, 3) ? 1 : A[2].i; /* step */
    aux[2] = A[3].i > 0 ? A[3].i : 1; /* rate */
    return push_view(v);
}

static int vops_view (lua_State *L) {
    LVQ_ARGS(L,A,"vv");
    if (A[0].v == 0)
        A[0].v = empty_meta(L);
    if (A[1].v != 0)
        A->v = vq_new(A[1].v, vwRows(A[0].v));
    return push_view(A->v);
}

static const struct luaL_reg lvqlib_vops[] = {
    {"call", vops_call},
    {"meta", vops_meta},
    {"step", vops_step},
    {"view", vops_view},
    {0, 0},
};
