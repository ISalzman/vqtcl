/*  Tcl extension binding.
    This file is part of LuaVlerq.
    See lvq.h for full copyright notice.
    $Id$  */

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

#include "lualib.h"
#include "lvq.c"

#define USE_TCL_STUBS 1
#include <tcl.h>

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
}

static int SetLuaFromAnyRep (Tcl_Interp *interp, Tcl_Obj *obj) {
    puts("SetLuaFromAnyRep called");
    return TCL_ERROR;
}

Tcl_ObjType f_luaObjType = {
    "luaobj", FreeLuaIntRep, DupLuaIntRep, UpdateLuaStrRep, SetLuaFromAnyRep
};

static Tcl_Obj *LuaAsTclObj (lua_State *L, int t) {
    Tcl_Obj *obj = Tcl_NewObj();
    Tcl_InvalidateStringRep(obj);
    lua_pushvalue(L, t);
    obj->internalRep.twoPtrValue.ptr1 = L;
    obj->internalRep.twoPtrValue.ptr2 = (void*) luaL_ref(L, LUA_REGISTRYINDEX);
    obj->typePtr = &f_luaObjType;
    return obj;
}

static int LuaCallback (lua_State *L) {
    Tcl_Obj **o = lua_touserdata(L, -2), *list = *o;
    Tcl_Interp* ip = lua_touserdata(L, -1);
    int i, n = lua_gettop(L) - 2;
    if (list == NULL || ip == NULL) {
        lua_pushstring(L, "LuaCallback not called with proper args");
        lua_error(L);
    }
    list = Tcl_DuplicateObj(list);
    Tcl_IncrRefCount(list);
    for (i = 1; i <= n; ++i) {
        Tcl_Obj* elem;
        if (lua_isnil(L, i)) {
            elem = Tcl_NewObj();
        } else if (lua_isnumber(L, i)) {
            double d = lua_tonumber(L, i);
            long l = (long) d;
            elem = l == d ? Tcl_NewLongObj(l) : Tcl_NewDoubleObj(d);
        } else if (lua_isstring(L, i)) {
            int len = lua_strlen(L, i);
            const char* ptr = lua_tostring(L, i);
            elem = Tcl_NewByteArrayObj((unsigned char*) ptr, len);
        } else
            elem = LuaAsTclObj(L, i);
        Tcl_ListObjAppendElement(ip, list, elem);
    }
    i = Tcl_EvalObjEx(ip, list, TCL_EVAL_DIRECT);
    Tcl_DecrRefCount(list);
    if (i != TCL_OK) {
        lua_pushstring(L, Tcl_GetStringResult(ip));
        lua_error(L);
    }
    return 0;
}

static int LuaObjCmd (ClientData data, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
#if 0
    if (objc > 1)
        luaL_dostring(data, Tcl_GetString(objv[1]));
    Tcl_AppendResult(interp, "bla bla", NULL);
    return TCL_OK;
#endif
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
                luaL_getmetatable(L, "LuaVlerq.tclobj");
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
    v = lua_pcall(L, objc - 3, 0, 0);
    if (v != 0) {
        Tcl_SetResult(interp, (char*) lua_tostring(L, -1), TCL_VOLATILE);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static void LuaDelProc (ClientData data) {
    lua_close(data);    
}

static int tclobj_jc (lua_State *L) {
    Tcl_Obj *p = lua_touserdata(L, -1);
    puts("tclobj: gc");
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

    luaL_newmetatable(L, "LuaVlerq.tclobj");
    lua_pushstring(L, "__gc");
    lua_pushcfunction(L, tclobj_jc);
    lua_settable(L, -3);

    Tcl_CreateObjCommand(interp, "tvq", LuaObjCmd, L, NULL);
    return Tcl_PkgProvide(interp, "tvq", PACKAGE_VERSION);
}
