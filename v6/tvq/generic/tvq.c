/*  Tcl extension binding.
    $Id$
    This file is part of Vlerq, see core/vlerq.h for full copyright notice.  */

#if defined(NDEBUG)

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

#endif

#define USE_TCL_STUBS 1
#include <tcl.h>

#include "lualib.h"
#include "lvq.c"

/* stub interface code, removes the need to link with libtclstub*.a */
#if defined(STATIC_BUILD)
#define MyInitStubs(x) 1
#else
#include "stubs.h"
#endif

/*  Define a custom "luaobj" type for Tcl objects, containing a reference to a
    Lua object.  The "L" state pointer is stored as twoPtrValue.ptr1, the int
    reference is stored as twoPtrValue.ptr2 (via a cast).  Calls Lua's unref
    when the Tcl_Obj is deleted.  The lifetime of the referenced Lua object is
    therefore tied to the Tcl_Obj's one.
    
    There is a string representation which is only for debugging - once the
    internal rep shimmers away, there is no way to get back the Lua object.  */
    
extern Tcl_ObjType f_luaObjType; /* forward */

static void FreeLuaIntRep (Tcl_Obj *obj) {
    lua_State *L = obj->internalRep.twoPtrValue.ptr1;
    int t = (int) obj->internalRep.twoPtrValue.ptr2;
    luaL_unref(L, LUA_REGISTRYINDEX, t);
}

static void DupLuaIntRep (Tcl_Obj *src, Tcl_Obj *obj) {
    puts("DupLuaIntRep called"); /* could be implemented, no need so far */
}

static void UpdateLuaStrRep (Tcl_Obj *obj) {
    char buf[50];
    int n = sprintf(buf, "luaobj %p %d",
                                    obj->internalRep.twoPtrValue.ptr1,
                                    (int) obj->internalRep.twoPtrValue.ptr2);
    obj->bytes = strcpy(malloc(n+1), buf);
    obj->length = n;
}

static int SetLuaFromAnyRep (Tcl_Interp *interp, Tcl_Obj *obj) {
    puts("SetLuaFromAnyRep called"); /* conversion to a luaobj is impossible */
    return TCL_ERROR;
}

Tcl_ObjType f_luaObjType = {
    "luaobj", FreeLuaIntRep, DupLuaIntRep, UpdateLuaStrRep, SetLuaFromAnyRep
};

static Tcl_Obj* LuaAsTclObj (lua_State *L, int t) {
    switch (lua_type(L, t)) {
        case LUA_TNIL:
            return Tcl_NewObj();
        case LUA_TNUMBER: {
            double d = lua_tonumber(L, t);
            long l = (long) d;
            return l == d ? Tcl_NewLongObj(l) : Tcl_NewDoubleObj(d);
        }
        case LUA_TSTRING: {
            int len = lua_strlen(L, t);
            const char* ptr = lua_tostring(L, t);
            return Tcl_NewByteArrayObj((unsigned char*) ptr, len);
        }
        case LUA_TBOOLEAN:
            return Tcl_NewBooleanObj(lua_toboolean(L, t));
        default: {
            Tcl_Obj *obj = Tcl_NewObj();
            Tcl_InvalidateStringRep(obj);
            obj->internalRep.twoPtrValue.ptr1 = L;
            lua_pushvalue(L, t);
            obj->internalRep.twoPtrValue.ptr2 = 
                                        (void*) luaL_ref(L, LUA_REGISTRYINDEX);
            obj->typePtr = &f_luaObjType;
            return obj;
        }
    }
}

/*  Define various Tcl conversions from/to Vlerq objects.  All calls below
    named ...As... return the result, while calls ...To... store it in arg.  */

vq_View ObjAsMetaView (void *ip, Tcl_Obj *obj) {
    int r, rows, objc;
    Object_p *objv, entry;
    const char *name, *sep;
    vq_Type type;
    vq_View table;

    if (Tcl_ListObjLength(ip, obj, &rows) != TCL_OK)
        return 0;

    table = vq_new(vMeta(EmptyMetaView()), rows);

    for (r = 0; r < rows; ++r) {
        Tcl_ListObjIndex(0, obj, r, &entry);
        if (Tcl_ListObjGetElements(ip, entry, &objc, &objv) != TCL_OK ||
                objc < 1 || objc > 2)
            return 0;

        name = Tcl_GetString(objv[0]);
        sep = strchr(name, ':');
        type = objc > 1 ? VQ_view : VQ_string;

        if (sep != 0) {
            int n = sep - name;
            char *buf = memcpy(malloc(n+1), name, n);
            buf[n] = 0;
            if (sep[1] != 0)
                type = StringAsType(sep+1);
            Vq_setString(table, r, 0, buf);
            free(buf);
        } else
            Vq_setString(table, r, 0, name);

        Vq_setInt(table, r, 1, type);

        if (objc > 1) {
            vq_View t = ObjAsMetaView(ip, objv[1]);
            if (t == 0)
                return 0;
            Vq_setView(table, r, 2, t);
        } else
            Vq_setView(table, r, 2, EmptyMetaView());
    }

    return table;
}

static Tcl_Obj* MetaViewAsList (vq_View meta) {
    Tcl_Obj *result = Tcl_NewListObj(0, 0);
    if (meta != 0) {
        vq_Type type;
        int rowNum;
        vq_View subt;
        Tcl_Obj *fieldobj;
        char buf[30];

        for (rowNum = 0; rowNum < vCount(meta); ++rowNum) {
            fieldobj = Tcl_NewStringObj(Vq_getString(meta, rowNum, 0, ""), -1);
            type = Vq_getInt(meta, rowNum, 1, VQ_nil);
            switch (type) {
                case VQ_string:
                    break;
                case VQ_view:
                    subt = Vq_getView(meta, rowNum, 2, 0);
                    assert(subt != 0);
                    if (vCount(subt) > 0) {
                        fieldobj = Tcl_NewListObj(1, &fieldobj);
                        Tcl_ListObjAppendElement(0, fieldobj,
                                                        MetaViewAsList(subt));
                        break;
                    }
                default:
                    Tcl_AppendToObj(fieldobj, ":", 1);
                    Tcl_AppendToObj(fieldobj, TypeAsString(type, buf), 1);
                    break;
            }
            Tcl_ListObjAppendElement(0, result, fieldobj);
        }
    }
    return result;
}

static Tcl_Obj* ColumnAsList (vq_Item colref, int rows, int mode) {
    int i;
    Tcl_Obj *list = Tcl_NewListObj(0, 0);
#if VQ_MOD_NULLABLE
    if (mode == 0) {
        Vector ranges = 0;
        for (i = 0; i < rows; ++i) {
            vq_Item item = colref;
            if (GetItem(i, &item) == VQ_nil)
                RangeFlip(&ranges, i, 1);
        }
        mode = -2;
        rows = vCount(ranges);
        colref.o.a.v = ranges;
    }
#endif
    for (i = 0; i < rows; ++i) {
        vq_Item item = colref;
        vq_Type type = GetItem(i, &item);
        if (mode < 0 || (mode > 0 && type != VQ_nil))
            Tcl_ListObjAppendElement(0, list, ItemAsObj(type, item));
        else if (mode == 0 && type == VQ_nil)
            Tcl_ListObjAppendElement(0, list, Tcl_NewIntObj(i));
    }
#if VQ_MOD_NULLABLE
    if (mode == -2)
        vq_release(colref.o.a.v);
#endif
    return list;
}

static Tcl_Obj* VectorAsList (Vector v) {
    vq_Item item;
    if (v == 0)
        return Tcl_NewObj();
    item.o.a.v = v;
    return ColumnAsList (item, vCount(v), -1);
}

Tcl_Obj* ChangesAsList (vq_View table) {
    int c, rows = vCount(table), cols = vCount(vq_meta(table));
    Tcl_Obj *result = Tcl_NewListObj(0, 0);
    if (IsMutable(table)) {
        Tcl_ListObjAppendElement(0, result, Tcl_NewStringObj("mutable", 7));
        Tcl_ListObjAppendElement(0, result, vOref(table));
        Tcl_ListObjAppendElement(0, result, VectorAsList(vDelv(table)));
        Tcl_ListObjAppendElement(0, result, VectorAsList(vPerm(table)));
        Tcl_ListObjAppendElement(0, result, VectorAsList(vInsv(table)));
        if (rows > 0)
            for (c = 0; c < cols; ++c) {
                Vector *vecp = (Vector*) vData(table) + 3 * c;
                Tcl_ListObjAppendElement(0, result, VectorAsList(vecp[0]));
                if (vecp[0] != 0 && vCount(vecp[0]) > 0) {
                    Tcl_ListObjAppendElement(0, result, VectorAsList(vecp[1]));
                    Tcl_ListObjAppendElement(0, result, VectorAsList(vecp[2]));
                }
            }
    }
    return result;
}

static Tcl_Obj* ViewAsList (vq_View table) {
    vq_View meta = vq_meta(table);
    int c, rows = vCount(table), cols = vCount(meta);
    Tcl_Obj *result = Tcl_NewListObj(0, 0);

    if (IsMutable(table)) {
        Tcl_DecrRefCount(result);
        result = ChangesAsList(table);
    } else if (meta == vq_meta(meta)) {
        if (rows > 0) {
            Tcl_ListObjAppendElement(0, result, Tcl_NewStringObj("mdef", 4));
            Tcl_ListObjAppendElement(0, result, MetaViewAsList(table));
        }
    } else if (cols == 0) {
        Tcl_ListObjAppendElement(0, result, Tcl_NewIntObj(rows));
    } else {
        Tcl_ListObjAppendElement(0, result, Tcl_NewStringObj("data", 4));
        Tcl_ListObjAppendElement(0, result, ViewAsList(meta));
        Tcl_ListObjAppendElement(0, result, Tcl_NewIntObj(rows));
        if (rows > 0)
            for (c = 0; c < cols; ++c) {
                int length;
                Tcl_Obj *list = ColumnAsList(table[c], rows, 1);
                Tcl_ListObjAppendElement(0, result, list);
                Tcl_ListObjLength(0, list, &length);
                if (length != 0 && length != rows) {
                    list = ColumnAsList(table[c], rows, 0);
                    Tcl_ListObjAppendElement(0, result, list);
                }
            }
    }
    return result;
}

Tcl_Obj* ItemAsObj (vq_Type type, vq_Item item) {
    switch (type) {
        case VQ_nil:    break;
        case VQ_int:    return Tcl_NewIntObj(item.o.a.i);
        case VQ_long:   return Tcl_NewWideIntObj(item.w);
        case VQ_float:  return Tcl_NewDoubleObj(item.o.a.f);
        case VQ_double: return Tcl_NewDoubleObj(item.d);
        case VQ_string: if (item.o.a.s == 0)
                            break;
                        return Tcl_NewStringObj(item.o.a.s, -1);
        case VQ_bytes:  if (item.o.a.p == 0)
                            break;
                        return Tcl_NewByteArrayObj(item.o.a.p, item.o.b.i);
        case VQ_view:   if (item.o.a.v == 0)
                            break;
                        return ViewAsList(item.o.a.v);/* FIXME: LuaAsTclObj ! */
        case VQ_object: return item.o.a.p;
    }
    return Tcl_NewObj();
}

int ObjToItem (vq_Type type, vq_Item *item) {
    switch (type) {
        case VQ_int:    return Tcl_GetIntFromObj(NULL, item->o.a.p,
                                                        &item->o.a.i) == TCL_OK;
        case VQ_long:   return Tcl_GetWideIntFromObj(NULL, item->o.a.p,
                                            (Tcl_WideInt*) &item->w) == TCL_OK;
        case VQ_float:
        case VQ_double: if (Tcl_GetDoubleFromObj(NULL, item->o.a.p,
                                                            &item->d) != TCL_OK)
                            return 0;
                        if (type == VQ_float)
                            item->o.a.f = (float) item->d;
                        break;
        case VQ_string: item->o.a.s = Tcl_GetString(item->o.a.p);
                        break;
        case VQ_bytes:  item->o.a.p = Tcl_GetByteArrayFromObj(item->o.a.p,
                                                                &item->o.b.i);
                        break;
        default:        return 0;
    }
    return 1;
}

/*  Callbacks from Lua into Tcl are implemented via the 'c' type in LvqCmd.
    The argument is treated as a list, to which any further arguments from
    Lua are appended.  That list is then eval'ed as a command in Tcl.  */

static int LuaCallback (lua_State *L) {
    Tcl_Obj *list, **o = lua_touserdata(L, lua_upvalueindex(1));
    Tcl_Interp* ip = lua_touserdata(L, lua_upvalueindex(2));
    int i, n = lua_gettop(L);
    assert(o != NULL);
    list = Tcl_DuplicateObj(*o);
    Tcl_IncrRefCount(list);
    for (i = 1; i <= n; ++i)
        Tcl_ListObjAppendElement(ip, list, LuaAsTclObj(L, i));
    i = Tcl_EvalObjEx(ip, list, TCL_EVAL_DIRECT);
    Tcl_DecrRefCount(list);
    if (i == TCL_ERROR)
        luaL_error(L, "tvq: %s", Tcl_GetStringResult(ip));
    return 0;
}

/*  Low-level interface from Tcl to Lua is via the "lvq" command.  Usage:
        set result [lvq fmt args...]
    The "fmt" argument is a string describing the type of the following args.
    Each letter describes one argument - if more arguments remain after parsing
    fmt, then these are treated as being of type 'u'.  Supported types are:
    
        b   arg is a Tcl byte array (pushed as Lua string)
        c   arg is a list for Lua to call back (pushed as a Lua C closure)
        d   arg is a double-precision float (pushed as Lua number)
        g   arg is name of a Lua global (looked up and pushed as Lua object)
        i   arg is an integer (pushed as Lua number)
        o   arg can be any Tcl object (pushed as Lua userdata of type Vlerq.tcl)
        s   arg is a UTF8 string (pushed as Lua string)
        t   arg is a Lua truth value, arg must be 1/0/yes/no/true/false
        u   arg can be any object (pushed as Lua light userdata)
    
    Args of type 'o' are checked - if they are of type "luaboj" then the value
    is "unboxed" and passed as that Lua object, otherwise the value is "boxed".
    This avoids multiple Tcl/Lua wrappers and supports passing back-and-forth.
    
    Args of type 'u' are pushed without incrementing the Tcl reference count
    and must be used right away in the Lua call - they cannot be kept around
    in Lua since the Tcl object may be released once LvqCmd returns.
    
    Examples:       lvq "gs" print {Tcl says hello to Lua!}
                    puts [lvq "ggs" rawget _G _VERSION]
*/
    
static int LvqCmd (ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
    lua_State *L = data;
    int v, i, len;
    char *ptr, *fmt;
    double d;
    Tcl_Obj **op;
    if (objc < 2) {
      Tcl_WrongNumArgs(interp, objc, objv, "fmt ?args ...?");
      return TCL_ERROR;
    }
    fmt = Tcl_GetStringFromObj(objv[1], &len);
    if (objc < 2 + len) {
        Tcl_SetResult(interp, "arg count mismatch", TCL_STATIC);
        return TCL_ERROR;
    }
    for (i = 2; i < objc; ++i) {
        Tcl_Obj *obj = objv[i];
        switch (*fmt++) {
            case 't':   if (Tcl_GetIntFromObj(interp, obj, &v) != TCL_OK)
                      	    return TCL_ERROR;
                        if (v >= 0)
                            lua_pushboolean(L, v);
                        else
                      	    lua_pushnil(L);
                      	break;
            case 'i':   if (Tcl_GetIntFromObj(interp, obj, &v) != TCL_OK)
                      	    return TCL_ERROR;
                      	lua_pushinteger(L, v);
                      	break;
            case 'd':   if (Tcl_GetDoubleFromObj(interp, obj, &d) != TCL_OK)
                      	    return TCL_ERROR;
                      	lua_pushnumber(L, d);
                      	break;
            case 'b':   ptr = (void*) Tcl_GetByteArrayFromObj(obj, &len);
                      	lua_pushlstring(L, ptr, len);
                      	break;
            case 's':   ptr = Tcl_GetStringFromObj(obj, &len);
                      	lua_pushlstring(L, ptr, len);
                      	break;
            case 'g':   ptr = Tcl_GetStringFromObj(obj, NULL);
                      	lua_getglobal(L, ptr);
                      	break;
            case 'o':   if (obj->typePtr == &f_luaObjType) { // unbox
                            int r = (int) obj->internalRep.twoPtrValue.ptr2;
                            lua_rawgeti(L, LUA_REGISTRYINDEX, r);
                            break;
                        } // else fall through
            case 'c':   op = newtypeddata(L, sizeof *op, "Vlerq.tcl");
                        *op = obj;
                      	Tcl_IncrRefCount(*op);
                        if (fmt[-1] == 'c') {
                          	lua_pushlightuserdata(L, interp);
                          	lua_pushcclosure(L, LuaCallback, 2);
                      	}
                      	break;
            case 0:     --fmt;
            case 'u':   lua_pushlightuserdata(L, obj);
                        break;
            default:    Tcl_SetResult(interp, "unknown format", TCL_STATIC);
                      	return TCL_ERROR;
        }
    }
    v = lua_pcall(L, objc-3, 1, 0);
    if (!lua_isnil(L, -1))
        Tcl_SetObjResult(interp, LuaAsTclObj(L, -1));
    lua_pop(L, 1);
    return v == 0 ? TCL_OK : TCL_ERROR;
}

static int TvqCmd (ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
    return TCL_OK;
}

static int tclobj_gc (lua_State *L) {
    Tcl_Obj *p = lua_touserdata(L, -1); assert(p != NULL);
    Tcl_DecrRefCount(p);
    return 0;
}

DLLEXPORT int Tvq_Init (Tcl_Interp *interp) {
    lua_State *L;
    
    if (!MyInitStubs(interp) || Tcl_PkgRequire(interp, "Tcl", "8.4", 0) == NULL)
        return TCL_ERROR;

    L = lua_open();
    Tcl_CreateExitHandler((Tcl_ExitProc*) lua_close, L);
    
    luaL_openlibs(L);

    luaL_newmetatable(L, "Vlerq.tcl");
    lua_pushstring(L, "__gc");
    lua_pushcfunction(L, tclobj_gc);
    lua_settable(L, -3);

    lua_pushcfunction(L, luaopen_lvq_core);
    lua_pushstring(L, "lvq.core");
    lua_call(L, 1, 0);

    Tcl_CreateObjCommand(interp, "lvq", LvqCmd, L, NULL);
    Tcl_CreateObjCommand(interp, "tvq", TvqCmd, L, NULL);
    
    assert(strcmp(PACKAGE_VERSION, VQ_RELEASE) == 0);
    return Tcl_PkgProvide(interp, "tvq", PACKAGE_VERSION);
}
