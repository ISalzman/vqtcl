/* lua.c -=- Lua interface to Thrive */

#define NAME "Lua"

/* %includeDir<.>% */
/* %includeDir<../noarch>% */
/* %include<thrive.h>% */
/* %include<>% */

#undef L
#include <lua.h>
#include <lauxlib.h>

#define THRIVE_WS  "thrive_ws*"
#define THRIVE_REF "thrive_ref*"

#define topws(L)	((P*)luaL_checkudata(L, 1, THRIVE_WS))
#define topref(L)	((P)luaL_checkudata(L, 1, THRIVE_REF))

static int ref_new(lua_State *L, P p, I i) {
  P x = (P)lua_newuserdata(L, sizeof(B));
  x->p = Work(p);
  x->i = makeRef(p,i);
  luaL_getmetatable(L, THRIVE_REF);
  lua_setmetatable(L, -2);
  return 1;
} 

static void th_get(lua_State *L, P p, I i) {
  P w = Work(p);
  if (i < 0) i += Length(p);
  switch (Type(p[i])) {
    case Tn:
      lua_pushnil(L);
      return;
    case Ti:
      lua_pushnumber(L, p[i].i);
      return;
    case Ts:
      lua_pushlstring(L, strAt(p[i]),lenAt(p[i]));
      return;
    case Ta: {
      int j, n = Length(p[i].p);
      lua_createtable(L, n, 0);
      if (n > Limit(p[i].p)) n = Limit(p[i].p);
      for (j = 0; j < n; ++j) {
	th_get(L,p[i].p,j);
	lua_rawseti(L, -2, j+1);
      }
      return;
    }
    default:
      ref_new(L,p,i);
  }
}

static void th_push(lua_State *L, P p, int o) {
  switch (lua_type(L, o)) {
    case LUA_TUSERDATA: {
      P x = (P)luaL_checkudata(L, o, THRIVE_REF);
      A(x->p == Work(p));
      pushRef(p,x->i);
      break;
    }
    case LUA_TNIL:
      nilPush(p,1);
      break;
    case LUA_TNUMBER:
      intPush(p,lua_tointeger(L, o));
      break;
    case LUA_TSTRING: {
      size_t n;
      S s = lua_tolstring(L, o, &n);
      strPush(p,s,n);
      break;
    }
    case LUA_TTABLE: {
      int i, n;
      P q;
      n = luaL_getn(L, o);
      q = boxPush(p,n);
      for (i = 1; i <= n; ++i) {
	lua_rawgeti(L, o, i);
	th_push(L, q, -1);
	lua_remove(L, -1);
      }
      break;
    }
    default:
      luaL_error(L, "cannot push, unknown type");
  }
}

static int ws_eval(lua_State *L) {
  P w = *topws(L);
  S s = luaL_checkstring(L, 2);
  int n;
  A(w != 0);
  strPush(w,s,~0);
  n = findSym(w);
  if (n >= 0) evalSym(w,n);
  lua_pushnumber(L, n);
  return 1;
}

static int ws_find(lua_State *L) {
  P w = *topws(L);
  S s = luaL_checkstring(L, 2);
  int n;
  A(w != 0);
  strPush(w,s,~0);
  n = findSym(w);
  /*TODO ugly code, breaks encapsulation of Wv */
  if (n >= Wv.i)
    luaL_error(L, "symbol not found");
  return ref_new(L,Wv.p,n);
}

static int ws_load(lua_State *L) {
  P w = *topws(L);
  S s = 0;
  M q, r;
  int n;
  A(w != 0);
  q = (M)luaL_checkstring(L, 2);
  r = q = strcpy(malloc(strlen(q)+1),q);
  if (*q != '\\' && *q != '#')
    for (;;) {
      s = scanSym(&r);
      if (s == 0) break;
      if (*s != '#') {
	strPush(w,s,~0);
	n = findSym(w);
	if (n >= 0) evalSym(w,n);
	if (runWork(w,0)>>1) {
	  luaL_error(L, "cannot handle callback");
	  break;
	}
      }
    }
  free(q);
  A(s == 0);
  return 0;
}

static int ws_pop(lua_State *L) {
  P w = *topws(L);
  A(w != 0);
  th_get(L, w, -1);
  return 1;
}

static int ws_push(lua_State *L) {
  P w = *topws(L);
  int i, n = lua_gettop(L);
  A(w != 0);
  for (i = 2; i <= n; ++i)
    th_push(L,w,i);
  return 0;
}

static int ws_run(lua_State *L) {
  P w = *topws(L);
  int f = lua_gettop(L) > 1;
  A(w != 0);
  if (f)
    th_push(L,w,2);
  lua_pushnumber(L, runWork(w,f));
  return 1;
}

static int ws_gc(lua_State *L) {
  P w = *topws(L);
  A(w != 0);
  endWork(w);
  return 0;
}

static int ws_tostring (lua_State *L) {
  P w = *topws(L);
  A(w != 0);
  lua_pushfstring(L, "thrive_ws (%p)", w);
  return 1;
}

static int ref_gc(lua_State *L) {
  P x = topref(L);
  A(x->p != 0);
  dropRef(x->p,x->i);
  return 0;
}

static int ref_tostring (lua_State *L) {
  P x = topref(L);
  A(x-> != 0);
  lua_pushfstring(L, "thrive_ref (%d @ %p)", x->i, x->p);
  return 1;
}

static const luaL_reg ws_meth[] = {
  { "eval", ws_eval },
  { "find", ws_find },
  { "load", ws_load },
  { "pop", ws_pop },
  { "push", ws_push },
  { "run", ws_run },
  {"__gc", ws_gc},
  {"__tostring", ws_tostring},
  {0,0}
};

static const luaL_reg ref_meth[] = {
  {"__gc", ref_gc},
  {"__tostring", ref_tostring},
  {0,0}
};

static int th_new(lua_State *L) {
  P* x = (P*)lua_newuserdata(L, sizeof(P));
  *x = newWork();
  luaL_getmetatable(L, THRIVE_WS);
  lua_setmetatable(L, -2);
  return 1;
}

static const luaL_reg thlib[] = {
  {"new", th_new},
  {0,0}
};

static void createmeta (lua_State *L) {
  luaL_newmetatable(L, THRIVE_WS);
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  luaL_register(L, NULL, ws_meth);
  luaL_newmetatable(L, THRIVE_REF);
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  luaL_register(L, NULL, ref_meth);
}

LUALIB_API int luaopen_thrive (lua_State *L) {
  luaL_register(L, "thrive", thlib);
  createmeta(L);
  return 1;
}

