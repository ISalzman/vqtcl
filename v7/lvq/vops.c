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
    LVQ_ARGS(L,A,"IVN");
    v = IndirectView(&cvtab, A[1].v, A[0].i, 0);
    assert(lua_gettop(L) == 3);
    vwAuxP(v) = (void*) luaL_ref(L, LUA_REGISTRYINDEX);
    return push_view(v);
}

static int vops_meta (lua_State *L) {
    LVQ_ARGS(L,A,"V");
    return push_view(vwMeta(A[0].v));
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
    {"view", vops_view},
    {0, 0},
};
