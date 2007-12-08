/*  Lua extension binding.
    $Id$
    This file is part of Vlerq, see core/vlerq.h for full copyright notice.  */

#include "vq_conf.h"

#include "vlerq.c"
#include "vopdef.c"
#include "vreader.c"
#include "vload.c"
#include "vbuffer.c"
#include "vsave.c"
#include "vranges.c"
#include "vmutable.c"

#if defined(VQ_EMBED_LUA)
#define luaall_c

#include "lapi.c"
#include "lcode.c"
#include "ldebug.c"
#include "ldo.c"
#include "ldump.c"
#include "lfunc.c"
#include "lgc.c"
#include "llex.c"
#include "lmem.c"
#include "lobject.c"
#include "lopcodes.c"
#include "lparser.c"
#include "lstate.c"
#include "lstring.c"
#include "ltable.c"
#include "ltm.c"
#include "lundump.c"
#include "lvm.c"
#include "lzio.c"

#include "lauxlib.c"
#include "lbaselib.c"
#include "ldblib.c"
#include "liolib.c"
#include "linit.c"
#include "lmathlib.c"
#include "loadlib.c"
#include "loslib.c"
#include "lstrlib.c"
#include "ltablib.c"

#else

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#endif

#define checkrow(L,t)   ((vq_Item*) luaL_checkudata(L, t, "Vlerq.row"))

LUA_API int luaopen_lvq_core (lua_State *L); /* forward */

static vq_View checkview (lua_State *L, int t) {
    switch (lua_type(L, t)) {
        case LUA_TNUMBER:
            return vq_new(luaL_checkinteger(L, t), NULL);
        case LUA_TSTRING:
            return DescToMeta(lua_tostring(L, t), -1);
    }
    return *(vq_View*) luaL_checkudata(L, t, "Vlerq.view");
}

static vq_View mustbemeta (lua_State *L, vq_View m) {
    if (vMeta(m) != vMeta(vMeta(m)))
        luaL_error(L, "arg must be a meta-view");
    return m;
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
        case VQ_objref: lua_rawgeti(L, LUA_REGISTRYINDEX, itemp->o.a.i); break;
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
        case VQ_string: /* fall through */
        case VQ_bytes:  item.o.a.s = luaL_checklstring(L, t, &n);
                        item.o.b.i = n; break;
        case VQ_view:   item.o.a.v = checkview(L, t); break;
        case VQ_objref: lua_pushvalue(L, t);
                        item.o.a.i = luaL_ref(L, LUA_REGISTRYINDEX);
                         /* FIXME: cleanup */
                        break;
    }

    return item;
}

static void parseargs(lua_State *L, vq_Item *buf, const char *desc) {
    int i;
    for (i = 0; desc[i]; ++i) {
        vq_Type type;
        char c = desc[i];
        if ('a' <= c && c <= 'z') {
            if (lua_isnoneornil(L, i+1)) {
                buf[i].o.a.p = buf[i].o.b.p = 0;
                continue;
            }
            c += 'A'-'a';
        }
        switch (c) {
            case 0:     return;
            case 'R':   buf[i] = *checkrow(L, i+1); continue;
            case '-':   continue;
            default:    type = CharAsType(c); break;
        }
        buf[i] = toitem(L, i+1, type);
    }
}

#define LVQ_ARGS(state,args,desc) \
            vq_Item args[sizeof(desc)-1]; \
            parseargs(state, args, desc)

/* - - - - - - - - - < Vlerq.row userdata methods > - - - - - - - - - */

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

/* - - - - - - - - - < Vlerq.view userdata methods > - - - - - - - - - */

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
        lua_getglobal(L, "vops");
        lua_getfield(L, -1, s);
        if (lua_isnil(L, -1))
            return luaL_error(L, "unknown '%s' view operator", s);
    }
    return 1;
}

static int view_len (lua_State *L) {
    LVQ_ARGS(L,A,"V");
    lua_pushinteger(L, vq_size(A[0].o.a.v));
    return 1;
}

static void view2struct (luaL_Buffer *B, vq_View meta) {
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
                    view2struct(B, m);
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
    view2struct(&b, vMeta(v));
    luaL_pushresult(&b);
    lua_concat(L, 2);
    return 1;
}

/* - - - - - - - - - < Remaining view operators > - - - - - - - - - */

static int vops_empty (lua_State *L) {
    LVQ_ARGS(L,A,"VII");
    lua_pushboolean(L, vq_empty(A[0].o.a.v, A[1].o.a.i, A[2].o.a.i));
    return 1;
}

static int vops_meta (lua_State *L) {
    LVQ_ARGS(L,A,"V");
    return pushview(L, vq_meta(A[0].o.a.v));
}

static int vops_type (lua_State *L) {
    LVQ_ARGS(L,A,"V");
    lua_pushstring(L, vType(A[0].o.a.v)->name);
    return 1;
}

static int vops_at (lua_State *L) {
    vq_View v;
    int r, c, cols;
    vq_Item item;
    LVQ_ARGS(L,A,"VII");
    v = A[0].o.a.v;
    r = A[1].o.a.i;
    c = A[2].o.a.i;
    cols = vCount(vMeta(v));
    if (r < 0 || r >= vCount(v))
        return luaL_error(L, "row index %d out of range", r);
    if (c < 0 || c >= cols)
        return luaL_error(L, "column index %d out of range", c);
    item = v[c];
    return pushitem(L, GetItem(r, &item), &item);
}

static int vops_replace (lua_State *L) {
    LVQ_ARGS(L,A,"VIIV");
    vq_replace(A[0].o.a.v, A[1].o.a.i, A[2].o.a.i, A[3].o.a.v);
    lua_pop(L, 3);
    return 1;
}

static int vops_struct (lua_State *L) {
    vq_View v;
    luaL_Buffer b;
    LVQ_ARGS(L,A,"V");
    v = A[0].o.a.v;
    luaL_buffinit(L, &b);
    view2struct(&b, vMeta(v));
    luaL_pushresult(&b);
    return 1;
}

#if VQ_LOAD_H

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

static int vops_open (lua_State *L) {
    Vector map;
    LVQ_ARGS(L,A,"S");
    map = OpenMappedFile(A[0].o.a.s);
    if (map == 0)
        return luaL_error(L, "cannot map '%s' file", A[0].o.a.s);
    return pushview(L, MapToView(map));
}

#endif

#if VQ_MUTABLE_H

static int vops_mutable (lua_State *L) {
    LVQ_ARGS(L,A,"V");
    return pushview(L, WrapMutable(A[0].o.a.v, NULL));
}

#endif

#if VQ_OPDEF_H

static vq_Type IotaGetter (int row, vq_Item *item) {
    item->o.a.i = row + vOffs(item->o.a.v);
    return VQ_int;
}
static Dispatch iotatab = {
    "iota", 3, 0, 0, IndirectCleaner, IotaGetter
};

vq_View IotaView (int rows, const char *name, int base) {
    vq_View v, meta = vq_new(1, vq_meta(0));
    Vq_setMetaRow(meta, 0, name, VQ_int, NULL);
    v = IndirectView(meta, &iotatab, rows, 0);
    vOffs(v) = base;
    return v;
}

static int vops_iota (lua_State *L) {
    vq_View v, meta;
    LVQ_ARGS(L,A,"VSi");
    meta = vq_new(1, vq_meta(0));
    Vq_setMetaRow(meta, 0, A[1].o.a.s, VQ_int, NULL);
    v = IndirectView(meta, &iotatab, vCount(A[0].o.a.v), 0);
    vOffs(v) = A[2].o.a.i;
    return pushview(L, v);
}

static int vops_pass (lua_State *L) {
    vq_View v, t;
    int c, cols;
    LVQ_ARGS(L,A,"V");
    v = A[0].o.a.v;
    t = vq_new(0, vMeta(v));
    cols = vCount(vMeta(v));
    vCount(t) = vCount(v);
    for (c = 0; c < cols; ++c) {
        t[c] = v[c];
        vq_retain(t[c].o.a.v);
    }
    return pushview(L, t);
}

static void VirtualCleaner (Vector v) {
    lua_unref((lua_State*) vData(v), vOffs(v));
    FreeVector(v);
}

static vq_Type VirtualGetter (int row, vq_Item *item) {
    vq_View v = item->o.a.v;
    int col = item->o.b.i;
    lua_State *L = (lua_State*) vData(v);
    lua_rawgeti(L, LUA_REGISTRYINDEX, vOffs(v));
    lua_pushinteger(L, row);
    lua_pushinteger(L, col);
    lua_call(L, 2, 1);
    if (lua_isnil(L, -1))
        return VQ_nil;
    *item = toitem(L, -1, Vq_getInt(vMeta(v), col, 1, VQ_nil) & VQ_TYPEMASK);
    lua_pop(L, 1);
    return VQ_int;
}
static Dispatch virtab = {
    "virtual", 3, 0, 0, VirtualCleaner, VirtualGetter
};

static int vops_virtual (lua_State *L) {
    vq_View v;
    LVQ_ARGS(L,A,"VVO");
    v = IndirectView(mustbemeta(L, A[1].o.a.v), &virtab, vCount(A[0].o.a.v), 0);
    vOffs(v) = A[2].o.a.i;
    vData(v) = (void*) L;
    return pushview(L, v);
}

#endif

#if VQ_SAVE_H

static void* EmitDataFun (void *buf, const void *ptr, intptr_t len) {
    luaL_addlstring(buf, ptr, len);
    return buf;
}

static int vops_emit (lua_State *L) {
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

static int vops_view (lua_State *L) {
    vq_View v;
    LVQ_ARGS(L,A,"Vv");
    v = A[0].o.a.v;
    if (A[1].o.a.v != NULL)
        v = vq_new(vCount(v), mustbemeta(L, A[1].o.a.v));
    return pushview(L, v);
}

/* - - - - - - - - - < Method registration tables > - - - - - - - - - */

static const struct luaL_reg lvqlib_row_m[] = {
    {"__gc", row_gc},
    {"__index", row_index},
    {"__newindex", row_newindex},
    {"__tostring", row2string},
    {NULL, NULL},
};

static const struct luaL_reg lvqlib_view_m[] = {
    {"__gc", view_gc},
    {"__index", view_index},
    {"__len", view_len},
    {"__tostring", view2string},
    {NULL, NULL},
};

static const struct luaL_reg lvqlib_v[] = {
    {"at", vops_at},
    {"empty", vops_empty},
    {"meta", vops_meta},
    {"replace", vops_replace},
    {"struct", vops_struct},
    {"type", vops_type},
    {"view", vops_view},
#if VQ_LOAD_H
    /* {"load", lvq_load}, */
    {"open", vops_open},
#endif
#if VQ_MUTABLE_H
    {"mutable", vops_mutable},
#endif
#if VQ_OPDEF_H
    {"iota", vops_iota},
    {"pass", vops_pass},
    {"virtual", vops_virtual},
#endif
#if VQ_SAVE_H
    {"emit", vops_emit},
#endif
    {NULL, NULL},
};

static const struct luaL_reg lvqlib_f[] = {
    {NULL, NULL},
};

LUA_API int luaopen_lvq_core (lua_State *L) {
    const char *sconf = "lvq " VQ_RELEASE
#if VQ_LOAD_H
                        " lo"
#endif
#if VQ_MUTABLE_H
                        " mu"
#endif
#if VQ_OPDEF_H
                        " op"
#endif
#if VQ_RANGES_H
                        " ra"
#endif
#if VQ_SAVE_H
                        " sa"
#endif
                        ;

    luaL_newmetatable(L, "Vlerq.row");
    luaL_register(L, NULL, lvqlib_row_m);
    
    luaL_newmetatable(L, "Vlerq.view");
    luaL_register(L, NULL, lvqlib_view_m);
    
    lua_newtable(L);
    luaL_register(L, NULL, lvqlib_v);
    lua_setglobal(L, "vops");

    luaL_register(L, "lvq", lvqlib_f);
    
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
