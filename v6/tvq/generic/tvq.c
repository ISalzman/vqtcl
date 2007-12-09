/*  Tcl extension binding.
    $Id$
    This file is part of Vlerq, see core/vlerq.h for full copyright notice.  */

#define USE_TCL_STUBS 1
#include <tcl.h>

#include "lvq.c"

static Tcl_Interp *context; /* FIXME: get rid of this global! */
    
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
    int ref = (int) obj->internalRep.twoPtrValue.ptr2;
    luaL_unref(L, LUA_REGISTRYINDEX, ref);
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
    Tcl_Obj **objv, *entry;
    const char *name, *sep;
    vq_Type type;
    vq_View table;

    if (Tcl_ListObjLength(ip, obj, &rows) != TCL_OK)
        return 0;

    table = vq_new(rows, vMeta(EmptyMetaView()));

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

static int AdjustCmdDef (Tcl_Obj *cmd) {
    Tcl_Obj *obj = Tcl_NewStringObj("tvq", 3);
    return Tcl_ListObjReplace(NULL, cmd, 0, 0, 1, &obj);
}

static vq_View ObjAsView (Tcl_Interp *interp, Tcl_Obj *obj) {
    int e = TCL_ERROR, objc, rows;
    Tcl_Obj **objv;
    vq_View view;
    
    if (obj->typePtr == &f_luaObjType) {
        lua_State *L = obj->internalRep.twoPtrValue.ptr1;
        int ref = (int) obj->internalRep.twoPtrValue.ptr2;
        lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
        view = checkview(L, lua_gettop(L));
        lua_pop(L, 1);
        return view;
    }
    
    if (Tcl_ListObjGetElements(interp, obj, &objc, &objv) != TCL_OK)
        return NULL;

    switch (objc) {

        case 0:
            return vq_new(0, NULL);

        case 1:
            if (Tcl_GetIntFromObj(interp, objv[0], &rows) == TCL_OK &&
                                                                rows >= 0) {
                return vq_new(rows, NULL);
            } else {
                const char *desc = Tcl_GetString(objv[0]);
                return DescToMeta(desc, -1);
            }
            
            break;

        default: {
            Tcl_Obj *cmd;
            Tcl_SavedResult state;

            assert(interp != NULL);
            Tcl_SaveResult(interp, &state);

            cmd = Tcl_DuplicateObj(obj);
            Tcl_IncrRefCount(cmd);
            view = NULL;

            /* def -> cmd namespace name conv can prob be avoided in Tcl 8.5 */
            if (AdjustCmdDef(cmd) == TCL_OK) {
                int ac;
                Tcl_Obj **av;
                Tcl_ListObjGetElements(NULL, cmd, &ac, &av);
                /* don't use Tcl_EvalObjEx, it forces a string conversion */
                if (Tcl_EvalObjv(interp, ac, av, TCL_EVAL_GLOBAL) == TCL_OK) {
                    /* result to view, may call EvalIndirectView recursively */
                    view = ObjAsView(interp, Tcl_GetObjResult(interp));
                }
            }

            Tcl_DecrRefCount(cmd);

            if (view == NULL) {
                Tcl_DiscardResult(&state);
                goto FAIL;
            }

            Tcl_RestoreResult(interp, &state);
            return view;
        }
    }

    e = TCL_OK;

FAIL:
    return NULL;
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
#if VQ_RANGES_H
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
#if VQ_RANGES_H
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
        case VQ_objref: return item.o.a.p;
    }
    return Tcl_NewObj();
}

int ObjToItem (vq_Type type, vq_Item *item) {
    switch (type) {
        case VQ_nil:    return 1;
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
        case VQ_view:   item->o.a.v = ObjAsView(context, item->o.a.p);
                        return item->o.a.v != NULL;
        case VQ_objref: assert(0); return 0;
    }
    return 1;
}

/*  Callbacks from Lua into Tcl are implemented via the 'c' type in LvqCmd.
    The argument is treated as a list, to which any further arguments from
    Lua are appended.  That list is then eval'ed as a command in Tcl.  */

static int LuaCallback (lua_State *L) {
    Tcl_Obj *list = Tcl_NewListObj(0, 0);
    Tcl_Interp* ip = lua_touserdata(L, lua_upvalueindex(1));
    int i, n = lua_gettop(L);
    Tcl_IncrRefCount(list);
    for (i = 1; i <= n; ++i)
        Tcl_ListObjAppendElement(ip, list, LuaAsTclObj(L, i));
    i = Tcl_EvalObjEx(ip, list, TCL_EVAL_DIRECT);
    Tcl_DecrRefCount(list);
    if (i == TCL_ERROR)
        luaL_error(L, "tvq: %s", Tcl_GetStringResult(ip));
    return 0;
}

static int TvqCmd (ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
    lua_State *L = data;
    int i, v;
    const char *cmd;

    if (objc < 2) {
      Tcl_WrongNumArgs(interp, objc, objv, "vop ?...?");
      return TCL_ERROR;
    }
    
    cmd = Tcl_GetStringFromObj(objv[1], NULL);
    lua_getglobal(L, "vops");
    lua_getfield(L, -1, cmd);
    lua_remove(L, -2);
    if (lua_isnil(L, -1)) {
        Tcl_AppendResult(interp, "not found in vops: ", cmd, NULL);
        return TCL_ERROR;
    }
                
    for (i = 2; i < objc; ++i)
        lua_pushlightuserdata(L, objv[i]);

    v = lua_pcall(L, objc-2, 1, 0);
    if (!lua_isnil(L, -1))
        Tcl_SetObjResult(interp, LuaAsTclObj(L, -1));
    lua_pop(L, 1);
    return v == 0 ? TCL_OK : TCL_ERROR;
}

static int tclobj_gc (lua_State *L) {
    Tcl_Obj *p = lua_touserdata(L, -1); assert(p != NULL);
    Tcl_DecrRefCount(p);
    return 0;
}

static int TvqDoLua (lua_State *L) {
    LVQ_ARGS(L,A,"S");
    if (luaL_loadstring(L, A[0].o.a.s))
        lua_error(L);
    lua_call(L, 0, 1);
    return 1;
}

DLLEXPORT int Tvq_Init (Tcl_Interp *interp) {
    lua_State *L;
    
    if (!MyInitStubs(interp) || Tcl_PkgRequire(interp, "Tcl", "8.4", 0) == NULL)
        return TCL_ERROR;

    context = interp;
    
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

  	lua_pushlightuserdata(L, interp);
  	lua_pushcclosure(L, LuaCallback, 1);
    lua_setglobal(L, "tcl");

    lua_getglobal(L, "vops");
    lua_pushcfunction(L, TvqDoLua);
    lua_setfield(L, -2, "dostring");
    lua_pop(L, 1);

    Tcl_CreateObjCommand(interp, "tvq", TvqCmd, L, NULL);
    
    assert(strcmp(PACKAGE_VERSION, VQ_RELEASE) == 0);
    return Tcl_PkgProvide(interp, "tvq", PACKAGE_VERSION);
}
