#include "vq.h"
#include "defs.h"

#include <lua.h>
#include <lauxlib.h>

LUA_API int (luaopen_vq) (lua_State *L); /* forward */

static vEnv *getEnvPtr (lua_State *L) {
    vEnv *env;
    lua_getfield(L, LUA_REGISTRYINDEX, "vq.env");
    env = lua_touserdata(L, -1);
    lua_pop(L, 1);
    assert(env != 0);
    return env;
}

/* ------------------------------------------------- conversions to/from Lua */

static void *tagged_udata (lua_State *L, size_t bytes, const char *tag) {
    void *ptr = lua_newuserdata(L, bytes);
    luaL_getmetatable(L, tag);
    lua_setmetatable(L, -2);
    return ptr;
}

static int push_column (lua_State *L, vColPtr cptr, int coff) {
    vAny *p = tagged_udata(L, sizeof *p, "vq.column");
    p->c = incRef(cptr);
    p->pair.two.j = coff;
    return 1;
}

static int push_row (lua_State *L, vView vw, int row) {
    vAny *p = tagged_udata(L, sizeof *p, "vq.row");
    p->c = incRef((vColPtr) vw);
    p->pair.two.j = row;
    return 1;
}

static int push_view (lua_State *L, vView vw) {
    vView *p = tagged_udata(L, sizeof *p, "vq.view");
    *p = (vView) incRef((vColPtr) vw);
    return 1;
}

static int push_type (lua_State *L, vType type, const vAny *val) {
    switch (type) {
        case v_NIL:
            lua_pushnil(L);
            break;
        case v_INT:
            lua_pushinteger(L, val->i);
            break;
        case v_POS:
            lua_pushinteger(L, val->i + 1);
            break;
        case v_LONG:
            lua_pushnumber(L, val->l);
            break;
        case v_FLOAT:
            lua_pushnumber(L, val->f);
            break;
        case v_DOUBLE:
            lua_pushnumber(L, val->d);
            break;
        case v_STRING:
            lua_pushstring(L, val->s);
            break;
        case v_BYTES:
            lua_pushlstring(L, val->p, val->pair.two.j);
            break;
        case v_VIEW:
            push_view(L, val->v);
            break;
    }
    return 1;
}

static int has_tag (lua_State *L, int t, const char *tag) {
    int f = 0;
    if (lua_getmetatable(L, t)) {
        lua_getfield(L, LUA_REGISTRYINDEX, tag);
        f = lua_rawequal(L, -1, -2);
        lua_pop(L, 2);
    }
    return f;
}

static vAny check_column (lua_State *L, int t) {
    /* also accept a view, which is a special variant of a column */
    if (has_tag(L, t, "vq.view")) {
        vAny val;
        /* avoid luaL_checkudata's redundant metatable checking */
        vView *p = lua_touserdata(L, t);
        val.v = *p;
        val.pair.two.j = -1;
        return val;
    } else {
        vAny *p = luaL_checkudata(L, t, "vq.column");
        return *p;
    }
}

static vAny check_row (lua_State *L, int t) {
    vAny *p = luaL_checkudata(L, t, "vq.row");
    return *p;
}

static vView check_view (lua_State *L, int t) {
    vView *p = luaL_checkudata(L, t, "vq.view");
    return *p;
}

static vAny check_type (lua_State *L, int t, vType type) {
    vAny value;
    switch (type) {
        case v_NIL:
            break;
        case v_INT:
            value.i = luaL_checkint(L, t);
            break;
        case v_POS:
            value.i = luaL_checkint(L, t) - 1;
            break;
        case v_LONG:
            value.l = (vLong) luaL_checknumber(L, t);
            break;
        case v_FLOAT:
            value.f = (float) luaL_checknumber(L, t);
            break;
        case v_DOUBLE:
            value.d = luaL_checknumber(L, t);
            break;
        case v_STRING:
            value.s = luaL_checkstring(L, t);
            break;
        case v_BYTES: {
            size_t len;
            value.p = (void*) luaL_checklstring(L, t, &len);
            value.pair.two.j = len;
            break;
        }
        case v_VIEW:
            value = check_column(L, t);
            break;
    }
    return value;
}

static vView as_view (lua_State *L, int t) {
    switch (lua_type(L, t)) {
        case LUA_TNIL:
            return vMETA(getEnvPtr(L)->empty);
        case LUA_TNUMBER:
            /* TODO careful with result, needs to incref/decref to clean up */
            return newView(getEnvPtr(L)->empty, luaL_checkint(L, t));
        case LUA_TSTRING:
            /* TODO not yet */
        default:
            return check_view(L, t);
    }
}

/* ---------------------------------------------------------- column objects */

static int column_len (lua_State *L) {
    vAny cptr = check_column(L, 1);
    lua_pushinteger(L, cptr.c->size);
    return 1;
}

static int column_index (lua_State *L) {
    vAny value = check_column(L, 1);
    int row = luaL_checkint(L, 2) - 1;
    assert(0 <= row && row < value.c->size);
    return push_type(L, (value.c->dispatch->getter)(row, &value), &value);
}

static int column_newindex (lua_State *L) {
    vAny col = check_column(L, 1);
    int row = luaL_checkint(L, 2) - 1;
    vAny value = check_type(L, 3, col.c->dispatch->type);
    assert(0 <= row && row < col.c->size);
    (col.c->dispatch->setter)(col.c, row, col.pair.two.j, &value);
    return 0;
}

static int column_gc (lua_State *L) {
    vAny cptr = check_column(L, 1);
    decRef(cptr.c);
    return 0;
}

static const struct luaL_reg column_methods[] = {
    {"__len", column_len},
    {"__index", column_index},
    {"__newindex", column_newindex},
    {"__gc", column_gc},
    {0, 0},
};

/* ------------------------------------------------------------- row objects */

static int row_gc (lua_State *L) {
    vAny rptr = check_row(L, 1);
    decRef(rptr.c);
    return 0;
}

static int row_index (lua_State *L) {
    vAny val, row = check_row(L, 1);
    int col = luaL_checkint(L, 2) - 1;
    return push_type(L, getItem(row.v, row.pair.two.j, col, &val), &val);
}

static int row_newindex (lua_State *L) {
    vAny row = check_row(L, 1);
    int col = luaL_checkint(L, 2) - 1;
    vAny value = check_type(L, 3, vCOLTYPE(row.v, col));
    setItem(row.v, row.pair.two.j, col, &value);
    return 0;
}

static const struct luaL_reg row_methods[] = {
    {"__gc", row_gc},
    {"__index", row_index},
    {"__newindex", row_newindex},
    {0, 0},
};

/* ------------------------------------------------------------ view objects */

static int view_len (lua_State *L) {
    vView vw = check_view(L, 1);
    lua_pushinteger(L, vROWS(vw));
    return 1;
}

static int view_index (lua_State *L) {
    const char* s;
    vView vw = check_view(L, 1);
    if (lua_isnumber(L, 2)) {
        int row = luaL_checkint(L, 2) - 1;
        return push_row(L, vw, row);
    }
    s = lua_tostring(L, 2);
    lua_getmetatable(L, 1);
    lua_getfield(L, -1, s);
    if (lua_isnil(L, -1))
        luaL_error(L, "unknown view operator: %s", s);
    return 1;
}

static int view_gc (lua_State *L) {
    vView vw = check_view(L, 1);
    decRef((vColPtr) vw);
    return 0;
}

static int view_meta (lua_State *L) {
    vView vw = check_view(L, 1);
    push_view(L, vMETA(vw));
    return 1;
}

static const struct luaL_reg view_methods[] = {
    {"__len", view_len},
    {"__index", view_index},
    {"__gc", view_gc},
    {"meta", view_meta},
    {0, 0},
};

/* ----------------------------------------------------- top-level functions */

static int vq_newcolumn (lua_State *L) {
    vEnv *env = getEnvPtr(L);
    vType type = luaL_checkint(L, 1);
    int size = luaL_checkint(L, 2);
    return push_column(L, newDataColumn(env, type, size), -1);
}

static int vq_newview (lua_State *L) {
    vView vw = as_view(L, 1);
    if (vISMETA(vw))
        vw = newView(vw, luaL_checkint(L, 2));
    return push_view(L, vw);
}

static int vq_info (lua_State *L) {
    vAny cptr = check_column(L, 1);
    lua_pushstring(L, cptr.c->dispatch->name);
    lua_pushinteger(L, cptr.c->dispatch->type);
    lua_pushinteger(L, cptr.pair.two.j + 1);
    return 3;
}

static int vq_refs (lua_State *L) {
    vAny cptr = check_column(L, 1);
    lua_pushinteger(L, cptr.c->refs);
    return 1;
}

static const struct luaL_reg vq_functions[] = {
    {"newcolumn", vq_newcolumn},
    {"newview", vq_newview},
    {"info", vq_info},
    {"refs", vq_refs},
    {0, 0},
};

/* ---------------------------------------------------------- extension init */

LUA_API int luaopen_vq (lua_State *L) {
    vType type;

    lua_pushlightuserdata(L, initEnv(L));
    lua_setfield(L, LUA_REGISTRYINDEX, "vq.env");

    luaL_newmetatable(L, "vq.column");
    luaL_register(L, 0, column_methods);

    luaL_newmetatable(L, "vq.row");
    luaL_register(L, 0, row_methods);

    luaL_newmetatable(L, "vq.view");
    luaL_register(L, 0, view_methods);

    luaL_register(L, "vq", vq_functions);

    lua_pushliteral(L, vCOPYRIGHT);
    lua_setfield(L, -2, "_COPYRIGHT");
    lua_pushliteral(L, "LuaVlerq " vVERSION);
    lua_setfield(L, -2, "_VERSION");
    
    /* define vq.N .. vq.V for easy access to type constants */
    for (type = v_NIL; type <= v_VIEW; ++type) {
        lua_pushlstring(L, vTYPES + type, 1);
        lua_pushinteger(L, type);
        lua_settable(L, -3);
    }

    return 1;
}
