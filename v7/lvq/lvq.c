/*  Lua extension binding.
    $Id$
    This file is part of Vlerq, see lvq/vlerq.h for full copyright notice. */

#include <lauxlib.h>

#include "vqbase.c"

#define checkrow(L,t)   ((vqCell*) luaL_checkudata(L, t, "lvq.row"))

static vqView check_view (lua_State *L, int t) {
    vqView v;

    switch (lua_type(L, t)) {
        case LUA_TNIL:      v = EmptyMeta(L);
                            break;
        case LUA_TBOOLEAN:  v = vq_new(EmptyMeta(L), lua_toboolean(L, t));
                            break;
        case LUA_TNUMBER:   v = vq_new(EmptyMeta(L), lua_tointeger(L, t));
                            break;
        case LUA_TSTRING:   v = AsMetaVop(L, lua_tostring(L, t));
                            break;
#if 0
        case LUA_TTABLE:    v = TableToView(L, t);
                            break;
#endif
        default:            v = luaL_checkudata(L, t, "lvq.view");
                            return v+1; /* views point past their header */
    }
    
    lua_replace(L, t);
    return v;
}

static int pushcell (lua_State *L, char c, vqCell *cp) {
    if (cp == 0)
        return 0;
        
    switch (c) {
        case 'N':   lua_pushnil(L); break;
        case 'I':   lua_pushinteger(L, cp->i); break;
        case 'L':   lua_pushnumber(L, cp->l); break;
        case 'F':   lua_pushnumber(L, cp->f); break;
        case 'D':   lua_pushnumber(L, cp->d); break;
        case 'S':   lua_pushstring(L, cp->s); break;
        case 'B':   lua_pushlstring(L, cp->p, cp->x.y.i); break;
        case 'V':   PushView(cp->v); break;
    /* pseudo-types */
        case 'T':   lua_pushboolean(L, cp->i); break;
        default:    assert(0);
    }

    return 1;
}

static vqType checkitem (lua_State *L, int t, char c, vqCell *cp) {
    size_t n;
    vqType type;

    if ('a' <= c && c <= 'z') {
        if (lua_isnoneornil(L, t)) {
            cp->p = cp->x.y.p = 0;
            return VQ_nil;
        }
        c += 'A'-'a';
    }
    
    type = CharAsType(c);
    
    switch (c) {
        case 'N':   break;
        case 'I':   cp->i = luaL_checkinteger(L, t); break;
        case 'L':   cp->l = (int64_t) luaL_checknumber(L, t); break;
        case 'F':   cp->f = (float) luaL_checknumber(L, t); break;
        case 'D':   cp->d = luaL_checknumber(L, t); break;
        case 'S':   /* fall through */
        case 'B':   cp->s = luaL_checklstring(L, t, &n);
                    cp->x.y.i = n; break;
        case 'V':   cp->v = check_view(L, t); break;
        default:    assert(0);
    }

    return type;
}

static void parseargs(lua_State *L, vqCell *buf, const char *desc) {
    int i;
    for (i = 0; *desc; ++i)
        checkitem(L, i+1, *desc++, buf+i);
}

#define LVQ_ARGS(state,args,desc) \
            vqCell args[sizeof(desc)-1]; \
            parseargs(state, args, desc)

static int row_gc (lua_State *L) {
    vqCell *pi = lua_touserdata(L, 1); assert(pi != NULL);
    /* ... */
    return 0;
}

static int rowcolcheck (lua_State *L, vqView *pv, int *pr) {
    vqView v, meta;
    int r, c, cols;
    vqCell *item = checkrow(L, 1);
    *pv = v = item->v;
    *pr = r = item->x.y.i;
    meta = vwMeta(v);
    cols = vwRows(meta);
    if (r < 0 || r >= vwRows(v))
        return luaL_error(L, "row index %d out of range", r);
    if (lua_isnumber(L, 2)) {
        c = lua_tointeger(L, 2);
        if (c < 0 || c >= cols)
            return luaL_error(L, "column index %d out of range", c);
    } else {
        const char *s = luaL_checkstring(L, 2);
        /* TODO: optimize this dumb linear search */
        for (c = 0; c < cols; ++c)
            if (strcmp(s, vq_getString(meta, c, 0, "")) == 0)
                return c;
        return luaL_error(L, "column '%s' not found", s);
    }   
    return c;
}

static int row_index (lua_State *L) {
    vqView v;
    int r, c = rowcolcheck(L, &v, &r);
    vqCell cell = vwCol(v,c);
    return pushcell(L, VQ_TYPES[GetCell(r, &cell)], &cell);
}

static int row_newindex (lua_State *L) {
    vqView v;
    vqCell item;
    vqType type = VQ_nil;
    int r, c = rowcolcheck(L, &v, &r);
    if (!lua_isnil(L, 3)) {
        type = vq_getInt(vwMeta(v), c, 1, VQ_nil) & VQ_TYPEMASK;
        type = checkitem(L, 3, VQ_TYPES[type], &item);
    }
    vq_set(v, r, c, type, item);
    return 0;
}

static int row2string (lua_State *L) {
    vqCell item = *checkrow(L, 1);
    lua_pushfstring(L, "row: %p %d", item.p, item.x.y.i);
    return 1;
}

static int view_gc (lua_State *L) {
    vqView v = lua_touserdata(L, 1); assert(v != 0);
    vDisp(v)->cleaner(v);
    return 0;
}

static int view_index (lua_State *L) {
    if (lua_isnumber(L, 2)) {
        vqCell *cp;
        LVQ_ARGS(L,A,"VI");
/*
        cp = newuserdata(L, sizeof *cp, "lvq.row");
        cp->v = vq_retain(A[0].v);
*/
        cp->x.y.i = A[1].i;
    } else {
        const char* s = luaL_checkstring(L, 2);
        lua_getglobal(L, "vops");
        lua_getfield(L, -1, s);
        if (lua_isnil(L, -1))
            return luaL_error(L, "unknown '%s' view operator", s);
    }
    return 1;
}

static int view_len (lua_State *L) {
    LVQ_ARGS(L,A,"V");
    lua_pushinteger(L, vwRows(A[0].v));
    return 1;
}

static void view2struct (luaL_Buffer *B, vqView meta) {
    char buf[30];
    int c;
    for (c = 0; c < vwRows(meta); ++c) {
        int type = vq_getInt(meta, c, 1, VQ_nil);
        if ((type & VQ_TYPEMASK) == VQ_view) {
            vqView m = vq_getView(meta, c, 2, 0);
            if (m != 0) {
                if (m == meta)
                    luaL_addchar(B, '^');
                else {
                    luaL_addchar(B, '(');
                    view2struct(B, m);
                    luaL_addchar(B, ')');
                }
                continue;
            }
        }
        luaL_addstring(B, TypeAsString(type, buf));
    }
}

static void PushStruct (vqView v) {
    luaL_Buffer b;
    luaL_buffinit(vwState(v), &b);
    view2struct(&b, vwMeta(v));
    luaL_pushresult(&b);
}

static int view2string (lua_State *L) {
    vqView v;
    LVQ_ARGS(L,A,"V");
    v = A[0].v;
    lua_pushfstring(L, "view.%s #%d ", vDisp(v)->name, vwRows(v));
    PushStruct(v);
    lua_concat(L, 2);
    return 1;
}

static const struct luaL_reg lvqlib_row_m[] = {
    {"__gc", row_gc},
    {"__index", row_index},
    {"__newindex", row_newindex},
    {"__tostring", row2string},
    {0, 0},
};

static const struct luaL_reg lvqlib_view_m[] = {
    {"__gc", view_gc},
    {"__index", view_index},
    {"__len", view_len},
    {"__tostring", view2string},
    {0, 0},
};

static const struct luaL_reg lvqlib_f[] = {
    {0, 0},
};

LUA_API int luaopen_lvq_core (lua_State *L) {
    luaL_newmetatable(L, "Vlerq.view");
    luaL_register(L, 0, lvqlib_view_m);
    
    InitEmpty(L);
    
    luaL_register(L, "lvq", lvqlib_f);
    lua_pushliteral(L, "LuaVlerq " VQ_VERSION);
    lua_setfield(L, -2, "_VERSION");
    lua_pushliteral(L, VQ_COPYRIGHT);
    lua_setfield(L, -2, "_COPYRIGHT");

    return 1;
}
