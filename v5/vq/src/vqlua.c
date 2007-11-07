/* Vlerq extension for Lua */

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

LUALIB_API int luaopen_vq5lua (lua_State *L);

static luaL_reg vq_reg[] = {
	{ NULL, NULL }
};

LUALIB_API int luaopen_vq5lua (lua_State *L) {
	luaL_openlib(L, "vq5lua", vq_reg, 0);
	return 1;
}
