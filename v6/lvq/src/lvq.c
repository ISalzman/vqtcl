/*  Lua extension binding.
    $Id$
    This file is part of Vlerq, see lvq.h for the full copyright notice.  */

#include "lua.h"
#include "lauxlib.h"
#include "lvq.h"

#include "vcore.c"
#include "vopdef.c"
#include "vreader.c"
#include "vload.c"
#include "vbuffer.c"
#include "vsave.c"
#include "vnullable.c"
#include "vmutable.c"

static vq_Item * checkrow (lua_State *L, int t) {
    void *ud = luaL_checkudata(L, t, "Vlerq.row");
    luaL_argcheck(L, ud != NULL, t, "'row' expected");
    return ud;
}

static vq_View checkview (lua_State *L, int t) {
    void *ud = luaL_checkudata(L, t, "Vlerq.view");
    luaL_argcheck(L, ud != NULL, t, "'view' expected");
    return *(vq_View*) ud;
}

static void *newtypeddata (lua_State *L, size_t bytes, const char *tag) {
    void *p = lua_newuserdata(L, bytes);
    luaL_getmetatable(L, tag);
    lua_setmetatable(L, -2);
    return p;
}

static int pushview (lua_State *L, vq_View v) {
    vq_View *vp = newtypeddata(L, sizeof *vp, "Vlerq.view");
    *vp = vq_retain(v);
    return 1;
}

static int pushitem (lua_State *L, vq_Type type, vq_Item *itemp) {
    if (itemp == NULL)
        return 0;
    switch (type) {
        case VQ_nil:    lua_pushnil(L); break;
        case VQ_int:    lua_pushinteger(L, itemp->o.a.i); break;
        case VQ_long:   lua_pushnumber(L, itemp->w); break;
        case VQ_float:  lua_pushnumber(L, itemp->o.a.f); break;
        case VQ_double: lua_pushnumber(L, itemp->d); break;
        case VQ_string: lua_pushstring(L, itemp->o.a.s); break;
        case VQ_bytes:  lua_pushlstring(L, itemp->o.a.p, itemp->o.b.i); break;
        case VQ_view:   pushview(L, itemp->o.a.v); break;
        case VQ_object: lua_pushnumber(L, -1); assert(0); break;
    }
    return 1;
}

static vq_Item toitem (lua_State *L, int t, vq_Type type) {
    vq_Item item;
    size_t n;

    if (lua_islightuserdata(L, t)) {
        item.o.a.p = lua_touserdata(L, t);
        if (ObjToItem(type, &item))
            return item;
    }

    switch (type) {
        case VQ_nil:    break;
        case VQ_int:    item.o.a.i = luaL_checkinteger(L, t); break;
        case VQ_long:   item.w = (int64_t) luaL_checknumber(L, t); break;
        case VQ_float:  item.o.a.f = (float) luaL_checknumber(L, t); break;
        case VQ_double: item.d = luaL_checknumber(L, t); break;
        case VQ_string: item.o.a.s = luaL_checkstring(L, t); break;
        case VQ_bytes:  item.o.a.s = luaL_checklstring(L, t, &n);
                        item.o.b.i = n; break;
        case VQ_view:   item.o.a.v = checkview(L, t); break;
        case VQ_object: assert(0); break;
    }

    return item;
}

static void parseargs(lua_State *L, vq_Item *buf, const char *desc) {
    int i;
    for (i = 0; desc[i]; ++i) {
        vq_Type type;
        switch (desc[i]) {
            case 0:     return;
            case 'R':   buf[i] = *checkrow(L, i+1); continue;
            case '-':   continue;
            default:    type = CharAsType(desc[i]); break;
        }
        buf[i] = toitem(L, i+1, type);
    }
}

#define LVQ_ARGS(state,args,desc) \
            vq_Item args[sizeof(desc)-1]; \
            parseargs(state, args, desc)

static int row_gc (lua_State *L) {
    vq_Item *pi = lua_touserdata(L, 1); assert(pi != NULL);
    vq_release(pi->o.a.v);
    return 0;
}

static int rowcolcheck (lua_State *L, vq_View *pv, int *pr) {
    vq_View v, meta;
    int r, c, cols;
    vq_Item *item = checkrow(L, 1);
    *pv = v = item->o.a.v;
    *pr = r = item->o.b.i;
    meta = vMeta(v);
    cols = vCount(meta);
    if (r < 0 || r >= vCount(v))
        return luaL_error(L, "row index %d out of range", r);
    if (lua_isnumber(L, 2)) {
        c = lua_tointeger(L, 2);
        if (c < 0 || c >= cols)
            return luaL_error(L, "column index %d out of range", c);
    } else {
        const char *s = luaL_checkstring(L, 2);
        /* TODO: optimize this dumb linear search */
        for (c = 0; c < cols; ++c)
            if (strcmp(s, Vq_getString(meta, c, 0, "")) == 0)
                return c;
        return luaL_error(L, "column '%s' not found", s);
    }   
    return c;
}

static int row_index (lua_State *L) {
    vq_View v;
    int r, c = rowcolcheck(L, &v, &r);
    vq_Item item = v[c];
    return pushitem(L, GetItem(r, &item), &item);
}

static int row_newindex (lua_State *L) {
    vq_View v;
    vq_Item item;
    vq_Type type = VQ_nil;
    int r, c = rowcolcheck(L, &v, &r);
    if (!lua_isnil(L, 3)) {
        type = Vq_getInt(vMeta(v), c, 1, VQ_nil) & VQ_TYPEMASK;
        item = toitem(L, 3, type);
    }
    vq_set(v, r, c, type, item);
    return 0;
}

static int row2string (lua_State *L) {
    LVQ_ARGS(L,A,"R");
    lua_pushfstring(L, "row %p %d", A[0].o.a.p, A[0].o.b.i);
    return 1;
}

static const struct luaL_reg vqlib_row_m[] = {
    {"__gc", row_gc},
    {"__index", row_index},
    {"__newindex", row_newindex},
    {"__tostring", row2string},
    {NULL, NULL},
};

static int view_empty (lua_State *L) {
    LVQ_ARGS(L,A,"VII");
    lua_pushboolean(L, vq_empty(A[0].o.a.v, A[1].o.a.i, A[2].o.a.i));
    return 1;
}

static int view_meta (lua_State *L) {
    LVQ_ARGS(L,A,"V");
    return pushview(L, vq_meta(A[0].o.a.v));
}

static int view_type (lua_State *L) {
    LVQ_ARGS(L,A,"V");
    lua_pushstring(L, vType(A[0].o.a.v)->name);
    return 1;
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
        rp = newtypeddata(L, sizeof *rp, "Vlerq.row");
        rp->o.a.v = vq_retain(A[0].o.a.v);
        rp->o.b.i = A[1].o.a.i;
    } else {
        const char* s = luaL_checkstring(L, 2);
        if (!luaL_getmetafield(L, 1, s))
            return luaL_error(L, "unknown '%s' view operator", s);
    }
    return 1;
}

static int view_len (lua_State *L) {
    LVQ_ARGS(L,A,"V");
    lua_pushinteger(L, vq_size(A[0].o.a.v));
    return 1;
}

static int view_replace (lua_State *L) {
    LVQ_ARGS(L,A,"VIIV");
    vq_replace(A[0].o.a.v, A[1].o.a.i, A[2].o.a.i, A[3].o.a.v);
    lua_pushvalue(L, 1);
    return 1;
}

static void viewstruct (luaL_Buffer *B, vq_View meta) {
    int c;
    char buf[30];
    for (c = 0; c < vCount(meta); ++c) {
        int type = Vq_getInt(meta, c, 1, VQ_nil);
        if ((type & VQ_TYPEMASK) == VQ_view) {
            vq_View m = Vq_getView(meta, c, 2, NULL);
            if (m != NULL) {
                if (m == meta)
                    luaL_addchar(B, '^');
                else {
                    luaL_addchar(B, '(');
                    viewstruct(B, m);
                    luaL_addchar(B, ')');
                }
                continue;
            }
        }
        luaL_addstring(B, TypeAsString(type, buf));
    }
}

static int view2string (lua_State *L) {
    vq_View v;
    luaL_Buffer b;
    LVQ_ARGS(L,A,"V");
    v = A[0].o.a.v;
    lua_pushfstring(L, "view %p #%d ", v, vq_size(v));
    luaL_buffinit(L, &b);
    viewstruct(&b, vMeta(v));
    luaL_pushresult(&b);
    lua_concat(L, 2);
    return 1;
}

#if VQ_MOD_LOAD

/*
vq_Type lvq_load (lua_State *L) {
    Vector map;
    Tcl_Obj *obj = a[0].o.a.p;
    ObjToItem(VQ_bytes, &a[0]);
    map = OpenMappedBytes(a[0].o.a.p, a[0].o.b.i, obj);
    if (map == 0)
        return VQ_nil;
    a->o.a.m = MapToTable(map);
    return VQ_table;
}
*/

static int lvq_open (lua_State *L) {
    Vector map;
    LVQ_ARGS(L,A,"S");
    map = OpenMappedFile(A[0].o.a.s);
    if (map == 0)
        return luaL_error(L, "cannot map '%s' file", A[0].o.a.s);
    return pushview(L, MapToView(map));
}

static int lvq_meta (lua_State *L) {
    LVQ_ARGS(L,A,"S");
    return pushview(L, DescToMeta(A[0].o.a.s, -1));
}

#endif

#if VQ_MOD_MUTABLE

static int view_mutable (lua_State *L) {
    LVQ_ARGS(L,A,"V");
    return pushview(L, WrapMutable(A[0].o.a.v, NULL));
}

#endif

#if VQ_MOD_OPDEF

static int view_iota (lua_State *L) {
    int base = luaL_optinteger(L, 3, 0);
    LVQ_ARGS(L,A,"VS");
    return pushview(L, IotaView(vq_size(A[0].o.a.v), A[1].o.a.s, base));
}

static int view_pass (lua_State *L) {
    LVQ_ARGS(L,A,"V");
    return pushview(L, PassView(A[0].o.a.v));
}

#endif

#if VQ_MOD_SAVE

static void* EmitDataFun (void *buf, const void *ptr, intptr_t len) {
    luaL_addlstring(buf, ptr, len);
    return buf;
}

static int view_emit (lua_State *L) {
    int e;
    luaL_Buffer b;
    LVQ_ARGS(L,A,"V");
    luaL_buffinit(L, &b);   
    e = ViewSave(A[0].o.a.v, &b, NULL, EmitDataFun) < 0;
    luaL_pushresult(&b);
    if (e)
        return luaL_error(L, "error in view emit");
    return 1;
}

#endif

static const struct luaL_reg vqlib_view_m[] = {
    {"empty", view_empty},
    {"meta", view_meta},
    {"replace", view_replace},
    {"type", view_type},
    {"__gc", view_gc},
    {"__index", view_index},
    {"__len", view_len},
    {"__tostring", view2string},
#if VQ_MOD_SAVE
    {"mutable", view_mutable},
#endif
#if VQ_MOD_OPDEF
    {"iota", view_iota},
    {"pass", view_pass},
#endif
#if VQ_MOD_SAVE
    {"emit", view_emit},
#endif
    {NULL, NULL},
};

static int lvq_view (lua_State *L) {
    int size = luaL_checkint(L, 1);
    vq_View meta = lua_isnoneornil(L, 2) ? NULL : checkview(L, 2);
    return pushview(L, vq_new(meta, size));
}

static const struct luaL_reg vqlib_f[] = {
    {"view", lvq_view},
#if VQ_MOD_LOAD
    /* {"load", lvq_load}, */
    {"meta", lvq_meta},
    {"open", lvq_open},
#endif
    {NULL, NULL},
};

int luaopen_lvq_core (lua_State *L) {
    const char *sconf = "lvq " VQ_RELEASE
#if VQ_MOD_LOAD
                        " lo"
#endif
#if VQ_MOD_MUTABLE
                        " mu"
#endif
#if VQ_MOD_OPDEF
                        " op"
#endif
#if VQ_MOD_NULLABLE
                        " nu"
#endif
#if VQ_MOD_SAVE
                        " sa"
#endif
                        ;

    luaL_newmetatable(L, "Vlerq.row");
    luaL_register(L, NULL, vqlib_row_m);
    
    luaL_newmetatable(L, "Vlerq.view");
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
