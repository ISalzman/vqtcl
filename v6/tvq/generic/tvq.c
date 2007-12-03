/*  Tcl extension binding.
    $Id$
    This file is part of Vlerq, see lvq.h for the full copyright notice.  */

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

/* forward */
extern Tcl_ObjType f_luaObjType;

static void FreeLuaIntRep (Tcl_Obj *obj) {
    lua_State *L = obj->internalRep.twoPtrValue.ptr1;
    int t = (int) obj->internalRep.twoPtrValue.ptr2;
    lua_unref(L, t);
}

static void DupLuaIntRep (Tcl_Obj *src, Tcl_Obj *obj) {
    puts("DupLuaIntRep called");
}

static void UpdateLuaStrRep (Tcl_Obj *obj) {
    char buf[50];
    int n = sprintf(buf, "luaobj#%d", (int) obj->internalRep.twoPtrValue.ptr2);
    obj->bytes = strcpy(malloc(n+1), buf);
    obj->length = n;
}

static int SetLuaFromAnyRep (Tcl_Interp *interp, Tcl_Obj *obj) {
    puts("SetLuaFromAnyRep called");
    return TCL_ERROR;
}

Tcl_ObjType f_luaObjType = {
    "luaobj", FreeLuaIntRep, DupLuaIntRep, UpdateLuaStrRep, SetLuaFromAnyRep
};

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

static Tcl_Obj* LuaAsTclObj (lua_State *L, int i) {
    Tcl_Obj* obj;
    if (lua_isnil(L, i)) {
        return Tcl_NewObj();
    }
    if (lua_isnumber(L, i)) {
        double d = lua_tonumber(L, i);
        long l = (long) d;
        return l == d ? Tcl_NewLongObj(l) : Tcl_NewDoubleObj(d);
    }
    if (lua_isstring(L, i)) {
        int len = lua_strlen(L, i);
        const char* ptr = lua_tostring(L, i);
        return Tcl_NewByteArrayObj((unsigned char*) ptr, len);
    }
    obj = Tcl_NewObj();
    Tcl_InvalidateStringRep(obj);
    lua_pushvalue(L, i);
    obj->internalRep.twoPtrValue.ptr1 = L;
    obj->internalRep.twoPtrValue.ptr2 = (void*) luaL_ref(L, LUA_REGISTRYINDEX);
    obj->typePtr = &f_luaObjType;
    return obj;
}

static int LuaCallback (lua_State *L) {
    Tcl_Obj **o = lua_touserdata(L, -2), *list = *o;
    Tcl_Interp* ip = lua_touserdata(L, -1);
    int i, n = lua_gettop(L) - 2;
    list = Tcl_DuplicateObj(list);
    Tcl_IncrRefCount(list);
    for (i = 1; i <= n; ++i)
        Tcl_ListObjAppendElement(ip, list, LuaAsTclObj(L, i));
    i = Tcl_EvalObjEx(ip, list, TCL_EVAL_DIRECT);
    Tcl_DecrRefCount(list);
    return i == TCL_OK ? 0 : luaL_error(L, "tvq: %s", Tcl_GetStringResult(ip));
}

static int LuaObjCmd (ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
    int v, i, len;
    char *ptr, *fmt;
    double d;
    lua_State *L = data;
    if (objc < 2) {
      Tcl_WrongNumArgs(interp, objc, objv, "fmt ?args ...?");
      return TCL_ERROR;
    }
    fmt = Tcl_GetStringFromObj(objv[1], &len);
    if (objc != 2 + len) {
        Tcl_SetResult(interp, "arg count mismatch", TCL_STATIC);
        return TCL_ERROR;
    }
    for (i = 2; *fmt; ++i) {
        Tcl_Obj *obj = objv[i];
        switch (*fmt++) {
            case 't':
              	if (Tcl_GetIntFromObj(interp, obj, &v) != TCL_OK)
              	    return TCL_ERROR;
                if (v >= 0)
                    lua_pushboolean(L, v);
                else
              	    lua_pushnil(L);
              	break;
            case 'i':
              	if (Tcl_GetIntFromObj(interp, obj, &v) != TCL_OK)
              	    return TCL_ERROR;
              	lua_pushinteger(L, v);
              	break;
            case 'd':
              	if (Tcl_GetDoubleFromObj(interp, obj, &d) != TCL_OK)
              	    return TCL_ERROR;
              	lua_pushnumber(L, d);
              	break;
            case 'r':
              	if (Tcl_GetIntFromObj(interp, obj, &v) != TCL_OK)
              	    return TCL_ERROR;
              	lua_rawgeti(L, LUA_REGISTRYINDEX, v);
              	break;
            case 'b':
              	ptr = (void*) Tcl_GetByteArrayFromObj(obj, &len);
              	lua_pushlstring(L, ptr, len);
              	break;
            case 's':
              	ptr = Tcl_GetStringFromObj(obj, &len);
              	lua_pushlstring(L, ptr, len);
              	break;
            case 'g':
              	ptr = Tcl_GetStringFromObj(obj, NULL);
              	lua_getglobal(L, ptr);
              	break;
            case 'o':
                if (obj->typePtr == &f_luaObjType) { // unbox
                    lua_State *L = obj->internalRep.twoPtrValue.ptr1;
                    int t = (int) obj->internalRep.twoPtrValue.ptr2;
                    lua_pushvalue(L, t);
                    break;
                } // else fall through
            case 'c': {
                Tcl_Obj **op = (Tcl_Obj**) lua_newuserdata(L, sizeof *op);
                *op = obj;
              	Tcl_IncrRefCount(*op);
                luaL_getmetatable(L, "Vlerq.tcl");
                lua_setmetatable(L, -2);
                if (fmt[-1] == 'c') {
                  	lua_pushlightuserdata(L, interp);
                  	lua_pushcclosure(L, LuaCallback, 2);
              	}
              	break;
          	}
            default:
              	Tcl_SetResult(interp, "unknown format specifier", TCL_STATIC);
              	return TCL_ERROR;
        }
    }
    v = lua_pcall(L, objc - 3, 1, 0);
    if (v != 0) {
        Tcl_SetResult(interp, (char*) lua_tostring(L, -1), TCL_VOLATILE);
        return TCL_ERROR;
    }
    if (!lua_isnil(L, -1))
        Tcl_SetObjResult(interp, LuaAsTclObj(L, -1));
    return TCL_OK;
}

static void LuaDelProc (ClientData data) {
    lua_close(data);    
}

static int tclobj_jc (lua_State *L) {
    Tcl_Obj *p = lua_touserdata(L, -1);
    assert(p != NULL);
    Tcl_DecrRefCount(p);
    return 0;
}

DLLEXPORT int Tvq_Init (Tcl_Interp *interp) {
    lua_State *L;
    
    if (!MyInitStubs(interp) || Tcl_PkgRequire(interp, "Tcl", "8.4", 0) == NULL)
        return TCL_ERROR;

    L = lua_open();
    Tcl_CreateExitHandler(LuaDelProc, L);
    
    luaL_openlibs(L);

    lua_pushcfunction(L, luaopen_lvq_core);
    lua_pushstring(L, "lvq.core");
    lua_call(L, 1, 0);
    luaL_dostring(L, "package.loaded['lvq.core'] = lvq");

    luaL_newmetatable(L, "Vlerq.tcl");
    lua_pushstring(L, "__gc");
    lua_pushcfunction(L, tclobj_jc);
    lua_settable(L, -3);

    luaL_dostring(L, "function dostring (x) return loadstring(x)() end");
    
    Tcl_CreateObjCommand(interp, "tvq", LuaObjCmd, L, NULL);
    return Tcl_PkgProvide(interp, "tvq", PACKAGE_VERSION);
}
