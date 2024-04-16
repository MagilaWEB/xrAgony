#include "lua.hpp"

extern "C"
{
#include "lfs.h"
#include "lmarshal.h"
}

#include "string.h"
#include "bit.h"
#include "math.h"
#include "table.h"
#include "functions_global.h"

extern "C" __declspec(dllexport) int luaopen_lua_extensions(lua_State * L)
{
	open_lfs(L);
	open_marshal(L);
	open_string(L);
	open_bit(L);
	open_math(L);
	open_table(L);
	open_functions_global(L);
	return 0;
}