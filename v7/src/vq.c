/*  Lua extension binding.
    $Id$
    This file is part of Vlerq, see src/vlerq.h for full copyright notice. */

#include <lauxlib.h>

#include "vlerq.h"
#include "defs.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core.c"

static int row_call (vqEnv env) {
    vqSlot *cp = lua_touserdata(env, 1);
    return push_view(cp->v);
}

static int row_gc (vqEnv env) {
    vqSlot *cp = checkrow(env, 1);
    vq_decref(cp->v);
    return 0;
}
static int rowcolcheck (vqEnv env, vqView *pv, int *pr) {
    vqView v, meta;
    int r, c, nc;
    vqSlot *cp = checkrow(env, 1);
    *pv = v = cp->v;
    *pr = r = cp->x.y.i;
    meta = vwMeta(v);
    nc = vSize(meta);
    if (r < 0)
        r += vSize(v);
    if (r < 0 || r >= vSize(v))
        return luaL_error(env, "row index %d out of range", r);
    if (lua_isnumber(env, 2)) {
        c = check_pos(env, 2);
        if (c < 0)
            c += vSize(meta);
        if (c < 0 || c >= nc)
            return luaL_error(env, "column index %d out of range", c);
    } else
        return colbyname(meta, luaL_checkstring(env, 2));
    return c;
}
static int row_len (vqEnv env) {
    vqSlot *cp = lua_touserdata(env, 1);
    push_pos(env, cp->x.y.i);
    return 1;
}
static int row_index (vqEnv env) {
    vqView v;
    int r, c = rowcolcheck(env, &v, &r);
    vqSlot cell = v->col[c];
    vqType type = getcell(r, &cell);
    return pushcell(env, VQ_TYPES[type], &cell);
}
static int row_newindex (vqEnv env) {
    vqView v;
    vqSlot cell;
    vqType type = VQ_nil;
    int r, c = rowcolcheck(env, &v, &r);
    if (!lua_isnil(env, 3)) {
        type = ViewColType(v, c);
        type = check_cell(env, 3, VQ_TYPES[type], &cell);
    }
    vq_set(v, r, c, type, cell);
    return 0;
}
static int row2string (vqEnv env) {
    vqSlot *cp = checkrow(env, 1);
    lua_pushfstring(env, "row: %d %s ", cp->x.y.i+1,
                                        vInfo(cp->v).dispatch->typename);
    push_struct(cp->v);
    lua_concat(env, 2);
    return 1;
}

static const struct luaL_reg vqlib_row_m[] = {
    {"__call", row_call},
    {"__gc", row_gc},
    {"__index", row_index},
    {"__len", row_len},
    {"__newindex", row_newindex},
    {"__tostring", row2string},
    {0, 0},
};

static int view_gc (vqEnv env) {
    vqView v = check_view(env, 1);
    /* FIXME problem in cleanup, something can't be deleted!!! vq_decref(v); */
    vq_decref(v);
    return 0;
}
static int view_index (vqEnv env) {
    assert(lua_isuserdata(env, 1));
    if (lua_isnumber(env, 2)) {
        vqSlot *cp;
        vqView v = check_view(env, 1);
        int i = check_pos(env, 2);
        if (i < 0)
            i += vSize(v);
        cp = tagged_udata(env, sizeof *cp, "vq.row");
        cp->v = vq_incref(v);
        cp->x.y.i = i;
    } else if (lua_isstring(env, 2)) {
        const char* s = lua_tostring(env, 2);
        lua_getmetatable(env, 1);
        lua_getfield(env, -1, s);
        if (lua_isnil(env, -1))
            return luaL_error(env, "unknown view operator: %s", s);
    } else {
        vqView v = check_view(env, 1), w = check_view(env, 2);
        vq_incref(v); vq_incref(w); /* TODO: cleanup, this is ugly */
        push_view(RowMapVop(v, w));
        vq_decref(v); vq_decref(w);
    }
    return 1;
}
static int view_len (vqEnv env) {
    lua_pushinteger(env, vSize(check_view(env, 1)));
    return 1;
}
static int view2string (vqEnv env) {
    vqView v = check_view(env, 1);
    lua_pushfstring(env, "view: %s #%d ", vInfo(v).dispatch->typename, vSize(v));
    push_struct(v);
    lua_concat(env, 2);
    return 1;
}

static const struct luaL_reg vqlib_view_m[] = {
    {"__gc", view_gc},
    {"__index", view_index},
    {"__len", view_len},
    {"__tostring", view2string},
    {"meta", o_meta},
    {0, 0},
};

/* cast all vop arguments to the proper type, then call the real vop */
static int vop_check (vqEnv env) {
    int i;
    const char *fmt = luaL_checkstring(env, lua_upvalueindex(1));
    lua_pushvalue(env, lua_upvalueindex(2));
    for (i = 0; fmt[i]; ++i)
        if (fmt[i] == '-')
            lua_pushvalue(env, i+1);
        else {
            vqSlot cell;
            vqType type = check_cell(env, i+1, fmt[i], &cell);
            pushcell(env, VQ_TYPES[type], &cell);
        }
    lua_call(env, i, 1);
    return 1;
}
/* define a vop as a C closure which first casts its args to the proper type */
static int vq_def (vqEnv env) {
    assert(lua_gettop(env) == 3);
    luaL_getmetatable(env, "vq.view");    /* a1 a2 a3 vops */
    lua_insert(env, 1);                   /* vops a1 a2 a3 */
    lua_pushcclosure(env, vop_check, 2);  /* vops a1 f */
    lua_settable(env, 1);                 /* vops */
    return 0;
}

static int vq_s2t (vqEnv env) {
    const char* s = luaL_checkstring(env, 1);
    lua_pushinteger(env, string2type(s));
    return 1;
}

static int vq_t2s (vqEnv env) {
    char buffer [30];
    vqType t = luaL_checkinteger(env, 1);
    lua_pushstring(env, type2string(t, buffer));
    return 1;
}

static const struct luaL_reg vqlib_f[] = {
    {"def", vq_def},
    {"s2t", vq_s2t},
    {"t2s", vq_t2s},
    {0, 0},
};

static int vq_call (vqEnv env) {
    vqView v;
    /* ignore arg 1, it's the vq module table */
    if (lua_istable(env, 2) && !lua_isnoneornil(env, 3)) {
        int r;
        vqSlot cell;
        vqType t1, t2 = VQ_nil;
        v = vq_new(check_view(env, 3), 1);
        vSize(v) = 0;
        t1 = ViewColType(v, 0);
        if (vwCols(v) > 1)
            t2 = ViewColType(v, 1);
        lua_pushnil(env);
        for (r = 0; lua_next(env, 2); ++r) {
            check_cell(env, -2, VQ_TYPES[t1], &cell);
            VecInsert(&v->col[0].c, r, 1);
            vq_set(v, r, 0, t1, cell);
            if (t2 != VQ_nil) {
                check_cell(env, -1, VQ_TYPES[t2], &cell);
                VecInsert(&v->col[1].c, r, 1);
                vq_set(v, r, 1, t2, cell);
            }
            lua_pop(env, 1);
        }
        vSize(v) = r;
    } else {
        vqView w = lua_isnoneornil(env, 3) ? 0 : check_view(env, 3);
        v = lua_isnoneornil(env, 2) ? empty_meta(env) : check_view(env, 2);
        if (w != 0) {
            vqView t = vq_incref(v); /* TODO: ugly, avoids leaking */
            int n = vSize(v);
            v = vq_new(w, n > 0 ? n : 1);
            vSize(v) = n; /* TODO: hack, to allow inserts in 0-row view */
            vq_decref(t);
        }
    }
    return push_view(v);
}

static const struct luaL_reg vqlib_mt[] = {
    {"__call", vq_call},
    {0, 0},
};

#if VQ_FULL
#include "mapped.c"
#include "ops.c"
#include "nulls.c"
#include "mutable.c"
#include "file.c"
#include "buffer.c"
#include "emit.c"
#include "sort.c"
#include "group.c"
#include "hash.c"
#endif

LUA_API int luaopen_vq_core (vqEnv env) {
    vqType t;
    
    lua_newtable(env); /* t */
    lua_pushstring(env, "v"); /* t s */
    lua_setfield(env, -2, "__mode"); /* t */
    lua_setfield(env, LUA_REGISTRYINDEX, "vq.pool"); /* <> */
    
    luaL_newmetatable(env, "vq.row");
    luaL_register(env, 0, vqlib_row_m);
#if VQ_FULL
    luaL_register(env, 0, vqlib_row_cmp_m);
#endif

    luaL_newmetatable(env, "vq.view");
    luaL_register(env, 0, vqlib_view_m);
#if VQ_FULL
    luaL_register(env, 0, vqlib_view_cmp_m);
    luaL_register(env, 0, vqlib_ops);
    luaL_register(env, 0, vqlib_nulls);
    luaL_register(env, 0, vqlib_mutable);
    luaL_register(env, 0, vqlib_emit);
    luaL_register(env, 0, vqlib_sort);
    luaL_register(env, 0, vqlib_group);
    luaL_register(env, 0, vqlib_hash);

    luaL_newmetatable(env, "vq.map");
    luaL_register(env, 0, vqlib_map_m);
#endif

    init_empty(env);
    
    luaL_register(env, "vq", vqlib_f);                            /* t */
#if VQ_FULL
    luaL_register(env, 0, vqlib_map_f);
#endif

    lua_pushliteral(env, VQ_COPYRIGHT);
    lua_setfield(env, -2, "_COPYRIGHT"); 
    lua_pushliteral(env, "vq " VQ_RELEASE 
                        " $Id$");
    lua_setfield(env, -2, "_RELEASE");
    lua_pushliteral(env, "LuaVlerq " VQ_VERSION);
    lua_setfield(env, -2, "_VERSION");
    
    lua_pushliteral(env, VQ_TYPES);
    lua_setfield(env, -2, "types");

    /* define vq.N .. vq.V for easy access to type constants */
    for (t = VQ_nil; t <= VQ_view; ++t) {
        lua_pushlstring(env, VQ_TYPES + t, 1);                    /* t s */
        lua_pushinteger(env, t);                                  /* t s i */
        lua_settable(env, -3);                                    /* t */
    }
    
    lua_newtable(env);
    luaL_register(env, 0, vqlib_mt);
    lua_setmetatable(env, -2);
    
    return 1;
}
