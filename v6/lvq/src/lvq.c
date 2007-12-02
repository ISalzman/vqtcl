/*  Lua extension binding.
    This file is part of LuaVlerq.
    See lvq.h for full copyright notice.
    $Id$  */

#include "lua.h"
#include "lauxlib.h"
#include "lvq.h"
#include "vcore.c"

#include <string.h>

static vq_Item * checkrow (lua_State *L, int t) {
    void *ud = luaL_checkudata(L, t, "LuaVlerq.row");
    luaL_argcheck(L, ud != NULL, t, "'row' expected");
    return ud;
}

static vq_View checkview (lua_State *L, int t) {
    void *ud = luaL_checkudata(L, t, "LuaVlerq.view");
    luaL_argcheck(L, ud != NULL, t, "'view' expected");
    return *(vq_View*) ud;
}

static int pushview (lua_State *L, vq_View v) {
    vq_View *vp = lua_newuserdata(L, sizeof *vp);
    *vp = vq_retain(v);
    luaL_getmetatable(L, "LuaVlerq.view");
    lua_setmetatable(L, -2);
    return 1;
}

static void parseargs(lua_State *L, vq_Item *buf, const char *desc) {
    int i;
    size_t n;
    for (i = 0; desc[i]; ++i)
        switch (desc[i]) {
            default:    assert(0);
            case 0:     return;
            case 'I':   buf[i].o.a.i = luaL_checkinteger(L, i+1); break;
            case 'D':   buf[i].d = luaL_checknumber(L, i+1); break;
            case 'S':   buf[i].o.a.s = luaL_checklstring(L, i+1, &n);
                        buf[i].o.b.i = n;
                        break;
            case 'R':   buf[i] = *checkrow(L, i+1); break;
            case 'V':   buf[i].o.a.m = checkview(L, i+1); break;
        }
}

#define LVQ_ARGS(L,args,desc) vq_Item args[10]; parseargs(L, args, desc)

static int row_call (lua_State *L) {
    LVQ_ARGS(L,A,"RI");
    lua_pushinteger(L, A[0].o.b.i * 1000 + A[1].o.a.i);
    return 1;
}

static int row_gc (lua_State *L) {
    vq_Item *vi = lua_touserdata(L, 1); assert(vi != NULL);
    vq_release(vi->o.a.m);
    return 0;
}

static int row_index (lua_State *L) {
    LVQ_ARGS(L,A,"RS");
    lua_pushinteger(L, strlen(A[1].o.a.s));
    return 1;
}

static int row2string (lua_State *L) {
    LVQ_ARGS(L,A,"R");
    lua_pushfstring(L, "row:%p,%d", A[0].o.a.p, A[0].o.b.i);
    return 1;
}

static const struct luaL_reg vqlib_row_m[] = {
    {"__call", row_call},
    {"__gc", row_gc},
    {"__index", row_index},
    {"__tostring", row2string},
    {NULL, NULL},
};

static int view_empty (lua_State *L) {
    LVQ_ARGS(L,A,"VII");
    lua_pushboolean(L, vq_empty(A[0].o.a.m, A[1].o.a.i, A[2].o.a.i));
    return 1;
}

static int view_iota (lua_State *L) {
    LVQ_ARGS(L,A,"VS");
    pushview(L, IotaView(vq_size(A[0].o.a.m), A[1].o.a.s));
    return 1;
}

static int view_meta (lua_State *L) {
    LVQ_ARGS(L,A,"V");
    pushview(L, vq_meta(A[0].o.a.m));
    return 1;
}

static int view_pass (lua_State *L) {
    vq_View orig, t;
    int c, cols;
    LVQ_ARGS(L,A,"V");
    orig = A[0].o.a.m;
    t = vq_new(vMeta(orig), 0);
    cols = vCount(vMeta(orig));
    vCount(t) = vCount(orig);
    for (c = 0; c < cols; ++c) {
        t[c] = orig[c];
        vq_retain(t[c].o.a.m);
    }
    pushview(L, t);
    return VQ_view;
}

static int view_gc (lua_State *L) {
    vq_View *vp = lua_touserdata(L, 1); assert(vp != NULL);
    vq_release(*vp);
    return 0;
}

static int view_index (lua_State *L) {
    if (lua_isnumber(L, 2)) {
        vq_Item *rp;
        LVQ_ARGS(L,A,"VI");
        rp = lua_newuserdata(L, sizeof *rp);
        rp->o.a.m = vq_retain(A[0].o.a.m);
        rp->o.b.i = A[1].o.a.i;
        luaL_getmetatable(L, "LuaVlerq.row");
        lua_setmetatable(L, -2);
    } else {
        const char* s = luaL_checkstring(L, 2);
        if (!luaL_getmetafield(L, 1, s)) {
            lua_pushfstring(L, "view operator not found: %s", s);
            lua_error(L);
        }
    }
    return 1;
}

static int view_len (lua_State *L) {
    LVQ_ARGS(L,A,"V");
    lua_pushinteger(L, vq_size(A[0].o.a.m));
    return 1;
}

static int view2string (lua_State *L) {
    LVQ_ARGS(L,A,"V");
    lua_pushfstring(L, "view:%p#%d:ISV", A[0].o.a.m, vq_size(A[0].o.a.m));
    return 1;
}

static const struct luaL_reg vqlib_view_m[] = {
    {"empty", view_empty},
    {"iota", view_iota},
    {"meta", view_meta},
    {"pass", view_pass},
    {"__gc", view_gc},
    {"__index", view_index},
    {"__len", view_len},
    {"__tostring", view2string},
    {NULL, NULL},
};

static int lvq_view (lua_State *L) {
    int size = luaL_checkint(L, 1);
    vq_View meta = lua_isnoneornil(L, 2) ? NULL : checkview(L, 2);
    pushview(L, vq_new(meta, size));
    return 1;
}

static const struct luaL_reg vqlib_f[] = {
    {"view", lvq_view},
    {NULL, NULL},
};

int luaopen_lvq_core (lua_State *L) {
    const char *sconf = "lvq " VQ_RELEASE
#if VQ_MOD_LOAD
                        " load"
#endif
#if VQ_MOD_MUTABLE
                        " mutable"
#endif
#if VQ_MOD_NULLABLE
                        " nullable"
#endif
#if VQ_MOD_SAVE
                        " save"
#endif
                        ;

    luaL_newmetatable(L, "LuaVlerq.row");
    luaL_register(L, NULL, vqlib_row_m);
    
    luaL_newmetatable(L, "LuaVlerq.view");
    luaL_register(L, NULL, vqlib_view_m);
    
    luaL_register(L, "lvq", vqlib_f);
    
    lua_pushliteral(L, "_CONFIG");
    lua_pushstring(L, sconf);
    lua_settable(L, -3);
    lua_pushliteral(L, "_COPYRIGHT");
    lua_pushliteral(L, VQ_COPYRIGHT);
    lua_settable(L, -3);
    lua_pushliteral(L, "_VERSION");
    lua_pushliteral(L, "LuaVlerq " VQ_VERSION);
    lua_settable(L, -3);
    
    return 1;
}
