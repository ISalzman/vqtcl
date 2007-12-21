/*  Lua extension binding.
    $Id$
    This file is part of Vlerq, see base/vlerq.h for full copyright notice. */

#include "vq_conf.h"

#include "vutil.c"
#include "vopdef.c"
#include "vreader.c"
#include "vload.c"
#include "vbuffer.c"
#include "vsave.c"
#include "vranges.c"
#include "vmutable.c"
#include "vlerq.c"
#include "vdispatch.c"

#if defined(VQ_EMBED_LUA)
#define luaall_c

/* this takes care of Windows, Mac OS X, and Linux builds */
#ifndef __WIN32
#define LUA_USE_POSIX 1
#endif

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

#define checkrow(L,t)   ((vq_Cell*) luaL_checkudata(L, t, "Vlerq.row"))

/* forward */
LUA_API int luaopen_lvq_core (lua_State *L);
static vq_View TableToView (lua_State *L, int t);

static vq_View checkview (lua_State *L, int t) {
    switch (lua_type(L, t)) {
        case LUA_TNUMBER:   return vq_new(luaL_checkinteger(L, t), NULL);
        case LUA_TSTRING:   return AsMetaVop(lua_tostring(L, t));
        case LUA_TTABLE:    return TableToView(L, t);
    }
    return *(vq_View*) luaL_checkudata(L, t, "Vlerq.view");
}

static vq_View mustbemeta (lua_State *L, vq_View m) {
    if (vMeta(m) != vMeta(vMeta(m)))
        luaL_error(L, "arg must be a meta-view");
    return m;
}

static void *newuserdata (lua_State *L, size_t bytes, const char *tag) {
    void *p = lua_newuserdata(L, bytes);
    luaL_getmetatable(L, tag);
    lua_setmetatable(L, -2);
    return p;
}

static int pushview (lua_State *L, vq_View v) {
    if (v == NULL) 
        lua_pushnil(L);
    else {
        vq_View *vp = newuserdata(L, sizeof *vp, "Vlerq.view");
        *vp = vq_retain(v);
    }
    return 1;
}

static int pushitem (lua_State *L, char c, vq_Cell *itemp) {
    if (itemp == NULL)
        return 0;
        
    switch (c) {
        case 'N':   lua_pushnil(L); break;
        case 'I':   lua_pushinteger(L, itemp->o.a.i); break;
        case 'L':   lua_pushnumber(L, itemp->w); break;
        case 'F':   lua_pushnumber(L, itemp->o.a.f); break;
        case 'D':   lua_pushnumber(L, itemp->d); break;
        case 'S':   lua_pushstring(L, itemp->o.a.s); break;
        case 'B':   lua_pushlstring(L, itemp->o.a.p, itemp->o.b.i); break;
        case 'V':   pushview(L, itemp->o.a.v); break;
        case 'O':   lua_rawgeti(L, LUA_REGISTRYINDEX, itemp->o.a.i); break;
    /* pseudo-types */
        case 'T':   lua_pushboolean(L, itemp->o.a.i != 0); break;
        default:    assert(0);
    }

    return 1;
}

static vq_Type checkitem (lua_State *L, int t, char c, vq_Cell *itemp) {
    size_t n;
    vq_Type type;

    if ('a' <= c && c <= 'z') {
        if (lua_isnoneornil(L, t)) {
            itemp->o.a.p = itemp->o.b.p = 0;
            return VQ_nil;
        }
        c += 'A'-'a';
    }
    
    type = CharAsType(c);
    
    if (lua_islightuserdata(L, t)) {
        itemp->o.a.p = lua_touserdata(L, t);
        if (ObjToItem(L, type, itemp))
            return type;
    }

    switch (c) {
        case 'N':   break;
        case 'I':   itemp->o.a.i = luaL_checkinteger(L, t); break;
        case 'L':   itemp->w = (int64_t) luaL_checknumber(L, t); break;
        case 'F':   itemp->o.a.f = (float) luaL_checknumber(L, t); break;
        case 'D':   itemp->d = luaL_checknumber(L, t); break;
        case 'S':   /* fall through */
        case 'B':   itemp->o.a.s = luaL_checklstring(L, t, &n);
                    itemp->o.b.i = n; break;
        case 'V':   itemp->o.a.v = checkview(L, t); break;
        case 'O':   lua_pushvalue(L, t);
                    itemp->o.a.i = luaL_ref(L, LUA_REGISTRYINDEX);
                    /* FIXME: reference is never released */
                    break;
        default:    assert(0);
    }

    return type;
}

static void parseargs(lua_State *L, vq_Cell *buf, const char *desc) {
    int i;
    for (i = 0; *desc; ++i)
        checkitem(L, i+1, *desc++, buf+i);
}

#define LVQ_ARGS(state,args,desc) \
            vq_Cell args[sizeof(desc)-1]; \
            parseargs(state, args, desc)

static vq_View TableToView (lua_State *L, int t) {
    vq_Cell item;
    vq_View m, v;
    int r, rows, c, cols;
    lua_getfield(L, t, "meta");
    m = lua_isnil(L, -1) ? AsMetaVop(":I") : checkview(L, -1);
    lua_pop(L, 1);
    cols = vCount(m);
    rows = cols > 0 ? lua_objlen(L, t) / cols : 0;
    v = vq_new(rows, m);
    for (c = 0; c < cols; ++c) {
        vq_Type type = vq_getInt(m, c, 1, VQ_nil) & VQ_TYPEMASK;
        for (r = 0; r < rows; ++r) {
            vq_Type ty = type;
            lua_pushinteger(L, r * cols + c + 1);
            lua_gettable(L, t);
            ty = lua_isnil(L, -1) ? VQ_nil
                                  : checkitem(L, -1, VQ_TYPES[type], &item);
            vq_set(v, r, c, ty, item);
            lua_pop(L, 1);
        }
    }
    return v;
}

/* ---------------------------------------------------------- VLERQ.ROW ----- */

static int row_gc (lua_State *L) {
    vq_Cell *pi = lua_touserdata(L, 1); assert(pi != NULL);
    vq_release(pi->o.a.v);
    return 0;
}

static int rowcolcheck (lua_State *L, vq_View *pv, int *pr) {
    vq_View v, meta;
    int r, c, cols;
    vq_Cell *item = checkrow(L, 1);
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
            if (strcmp(s, vq_getString(meta, c, 0, "")) == 0)
                return c;
        return luaL_error(L, "column '%s' not found", s);
    }   
    return c;
}

static int row_index (lua_State *L) {
    vq_View v;
    int r, c = rowcolcheck(L, &v, &r);
    vq_Cell item = v[c];
    return pushitem(L, VQ_TYPES[GetItem(r, &item)], &item);
}

static int row_newindex (lua_State *L) {
    vq_View v;
    vq_Cell item;
    vq_Type type = VQ_nil;
    int r, c = rowcolcheck(L, &v, &r);
    if (!lua_isnil(L, 3)) {
        type = vq_getInt(vMeta(v), c, 1, VQ_nil) & VQ_TYPEMASK;
        type = checkitem(L, 3, VQ_TYPES[type], &item);
    }
    vq_set(v, r, c, type, item);
    return 0;
}

static int row2string (lua_State *L) {
    vq_Cell item = *checkrow(L, 1);
    lua_pushfstring(L, "row: %p %d", item.o.a.p, item.o.b.i);
    return 1;
}

/* --------------------------------------------------------- VLERQ.VIEW ----- */

static int view_gc (lua_State *L) {
    vq_View *vp = lua_touserdata(L, 1); assert(vp != NULL);
    vq_release(*vp);
    return 0;
}

static int view_index (lua_State *L) {
    if (lua_isnumber(L, 2)) {
        vq_Cell *rp;
        LVQ_ARGS(L,A,"VI");
        rp = newuserdata(L, sizeof *rp, "Vlerq.row");
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
        int type = vq_getInt(meta, c, 1, VQ_nil);
        if ((type & VQ_TYPEMASK) == VQ_view) {
            vq_View m = vq_getView(meta, c, 2, NULL);
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
    lua_pushfstring(L, "view: %p #%d ", v, vq_size(v));
    luaL_buffinit(L, &b);
    view2struct(&b, vMeta(v));
    luaL_pushresult(&b);
    lua_concat(L, 2);
    return 1;
}

/* ------------------------------------------------------ VIRTUAL VIEWS ----- */

static void VirtualCleaner (Vector v) {
    lua_unref((lua_State*) vData(v), vOffs(v));
    FreeVector(v);
}

static vq_Type VirtualGetter (int row, vq_Cell *item) {
    vq_Type type;
    vq_View v = item->o.a.v;
    int col = item->o.b.i;
    lua_State *L = (lua_State*) vData(v);
    lua_rawgeti(L, LUA_REGISTRYINDEX, vOffs(v));
    lua_pushinteger(L, row);
    lua_pushinteger(L, col);
    lua_call(L, 2, 1);
    if (lua_isnil(L, -1))
        return VQ_nil;
    type = vq_getInt(vMeta(v), col, 1, VQ_nil) & VQ_TYPEMASK;
    type = checkitem(L, -1, VQ_TYPES[type], item);
    lua_pop(L, 1);
    return type;
}
static Dispatch virtab = {
    "virtual", 3, 0, 0, VirtualCleaner, VirtualGetter
};

static vq_View VirtualView (int rows, vq_View meta, lua_State *L, int ref) {
    vq_View v = IndirectView(mustbemeta(L, meta), &virtab, rows, 0);
    vOffs(v) = ref;
    vData(v) = (void*) L;
    return v;
}

static int vops_virtual (lua_State *L) {
    int rows;
    LVQ_ARGS(L,A,"VVO");
    rows = vCount(A[0].o.a.v);
    return pushview(L, VirtualView(rows, A[1].o.a.v, L, A[2].o.a.i));
}

/* ----------------------------------------------------- VIEW OPERATORS ----- */

static int vops_at (lua_State *L) {
    vq_View v;
    int r, c, cols;
    vq_Cell item;
    LVQ_ARGS(L,A,"VII");
    v = A[0].o.a.v;
    r = A[1].o.a.i;
    c = A[2].o.a.i;
    cols = vCount(vMeta(v));
    if (r < 0 || r >= vCount(v))
        return luaL_error(L, "row index out of range");
    if (c < 0 || c >= cols)
        return luaL_error(L, "column index out of range");
    item = v[c];
    return pushitem(L, VQ_TYPES[GetItem(r, &item)], &item);
}

static int vops_row (lua_State *L) {
    vq_Cell *rp;
    LVQ_ARGS(L,A,"VI");
    rp = newuserdata(L, sizeof *rp, "Vlerq.row");
    rp->o.a.v = vq_retain(A[0].o.a.v);
    rp->o.b.i = A[1].o.a.i;
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

#if VQ_SAVE_H

static void* EmitDataFun (void *buf, const void *ptr, intptr_t len) {
    luaL_addlstring(buf, ptr, len);
    return buf;
}

static int vops_emit (lua_State *L) {
    int n;
    luaL_Buffer b;
    LVQ_ARGS(L,A,"V");
    luaL_buffinit(L, &b);   
    n = ViewSave(A[0].o.a.v, &b, NULL, EmitDataFun);
    luaL_pushresult(&b);
    if (n < 0)
        return luaL_error(L, "error in view emit");
    return 1;
}

#endif

/* ------------------------------------------------------ METHOD TABLES ----- */

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

static const struct luaL_reg lvqlib_vops[] = {
    {"at", vops_at},
    {"row", vops_row},
    {"struct", vops_struct},
#if VQ_OPDEF_H
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

/* cast all vop arguments to the proper type, then call the real vop */
static int vop_check (lua_State *L) {
    int i;
    const char *fmt = luaL_checkstring(L, lua_upvalueindex(1));
    lua_pushvalue(L, lua_upvalueindex(2));
    for (i = 0; fmt[i]; ++i)
        if (fmt[i] == '-')
            lua_pushvalue(L, i+1);
        else {
            vq_Cell item;
            vq_Type type = checkitem(L, i+1, fmt[i], &item);
            pushitem(L, VQ_TYPES[type], &item);
        }
    lua_call(L, i, 1);
    return 1;
}

/* define a vop as a C closure which first casts its args to the proper type */
static int vop_def (lua_State *L) {
    assert(lua_gettop(L) == 3);
    lua_getglobal(L, "vops");           /* a1 a2 a3 vops */
    lua_insert(L, 1);                   /* vops a1 a2 a3 */
    lua_pushcclosure(L, vop_check, 2);  /* vops a1 f */
    lua_settable(L, 1);                 /* vops */
    return 0;
}

/* this union is needed to avoid ISO C warnings w.r.t function vs. void ptrs */
typedef union { void *p; vq_Type (*f)(vq_Cell*); } vq_FunPtrCast;

/* collect remaining arguments as vectors */
static void collect_args (lua_State *L, int t, const char* fmt, int argc, vq_Cell *argv) {
    vq_Cell item;
    int i, cols = strlen(fmt), rows;
    assert(cols > 0 && argc % cols == 0);
    rows = argc / cols;
    /* TODO: clean up these new vectors somewhere... */
    for (i = 0; i < cols; ++i)
        argv[i].o.a.v = AllocDataVec(CharAsType(fmt[i]), rows);
    for (i = 0; i < argc; ++i) {
        int col = i % cols;
        vq_Type type = checkitem(L, t+i, fmt[col], &item);
        Vector v = argv[col].o.a.v;
        vType(v)->setter(v, i/cols, col, type != VQ_nil ? &item : 0);        
    }
}

/* cast all vop arguments to the proper type, then call the real C code */
static int vop_ccall (lua_State *L) {
    int i;
    vq_Cell args [10];
    vq_FunPtrCast fp;
    const char *fmt = luaL_checkstring(L, lua_upvalueindex(1)), *afmt = fmt+2;
    assert(sizeof fp.p == sizeof fp.f);
    fp.p = lua_touserdata(L, lua_upvalueindex(2));
    for (i = 0; *afmt; ++i) {
        if (*afmt == '_') {
            collect_args(L, i+1, ++afmt, lua_gettop(L)-i, args+i);
            break;
        }
        checkitem(L, i+1, *afmt++, args+i);
    }
    (fp.f)(args);
    return pushitem(L, *fmt, args);
}

static void register_vops (lua_State *L, CmdDispatch *defs) {
    vq_FunPtrCast fp;
    int t = lua_gettop(L);
    while (defs->name != NULL) {
        fp.f = defs->proc;
        lua_pushstring(L, defs->args);      /* t args */
        lua_pushlightuserdata(L, fp.p);     /* t args proc */
        lua_pushcclosure(L, vop_ccall, 2);  /* t f */
        lua_setfield(L, t, defs->name);     /* t */
        ++defs;
    }
}

LUA_API int luaopen_lvq_core (lua_State *L) {
    const char *sconf = "lvq " VQ_RELEASE
#if VQ_DISPATCH_H
                        " di"
#endif
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
#if VQ_UTIL_H
                        " ut"
#endif
                        ;

    luaL_newmetatable(L, "Vlerq.row");
    luaL_register(L, NULL, lvqlib_row_m);
    
    luaL_newmetatable(L, "Vlerq.view");
    luaL_register(L, NULL, lvqlib_view_m);
    
    lua_newtable(L);
    register_vops(L, f_vdispatch);
    luaL_register(L, NULL, lvqlib_vops);
    lua_setglobal(L, "vops");

    lua_register(L, "vopdef", vop_def);
    
    luaL_register(L, "lvq", lvqlib_f);
    
    lua_pushstring(L, sconf);
    lua_setfield(L, -2, "_CONFIG");
    lua_pushliteral(L, VQ_COPYRIGHT);
    lua_setfield(L, -2, "_COPYRIGHT");
    lua_pushliteral(L, "LuaVlerq " VQ_VERSION);
    lua_setfield(L, -2, "_VERSION");
    
    return 1;
}
