/*
* EXTENDED_LUA
* Implementation of additional global functions.
* The code is aimed at expanding the capabilities of luajit.
* Kirill Belash Aleksandrovich <belash.kirill2017@yandex.ru>
*
* Copyright (c) 2023 Kirill Belash Aleksandrovich ( Nickname: MAGILA )
*
* Last edit: 27.11.2023
*/
#include "lua.hpp"
#include <string>
#include "functions_global.h"

void set_info_extend_lua(lua_State* L) {
	lua_pushliteral(L, "PLUGIN_EXTENDED_LUA_COPYRIGHT");
	lua_pushliteral(L, "Copyright (C) MAGILA.");
	lua_settable(L, -3);
	lua_pushliteral(L, "PLUGIN_EXTENDED_LUA_DESCRIPTION");
	lua_pushliteral(L, "EXTENDED_LUA is a Lua library that extends the capabilities of Lua through additional fast global functions that are not enough in standard luajit v2.1.");
	lua_settable(L, -3);
	lua_pushliteral(L, "PLUGIN_EXTENDED_LUA_VERSION");
	lua_pushliteral(L, "EXTENDED_LUA v0.1.3 (27.11.2023)");
	lua_settable(L, -3);
}

/*
** Converting various data types to bool type.
*/
int to_bool(lua_State* L)
{
	switch (lua_type(L, 1))
	{
	case LUA_TBOOLEAN: {
		lua_pushboolean(L, lua_toboolean(L, -1));
		break;
	}
	case LUA_TSTRING: {
		size_t l;
		const char* str_val = lua_tolstring(L, -1, &l);
		if
			(
				_stricmp(str_val, "true") == 0 ||
				_stricmp(str_val, "1") == 0 ||
				_stricmp(str_val, "on") == 0 ||
				_stricmp(str_val, "yes") == 0
				)
			lua_pushboolean(L, 1);
		else
			lua_pushboolean(L, 0);
		break;
	}
	case LUA_TNUMBER:
	{
		lua_pushboolean(L, lua_tonumber(L, -1) >= 1);
		break;
	}
	default:
		lua_pushboolean(L, 0);
	}

	return 1;
}

/*
** Quick data type checking with boolean value return.
*/
int is_string(lua_State* L)
{
	lua_pushboolean(L, lua_isstring(L, 1));
	return 1;
}

int is_number(lua_State* L)
{
	lua_pushboolean(L, lua_isnumber(L, 1));
	return 1;
}

int is_function(lua_State* L)
{
	lua_pushboolean(L, lua_isfunction(L, 1));
	return 1;
}

int is_table(lua_State* L)
{
	lua_pushboolean(L, lua_istable(L, 1));
	return 1;
}

int is_bool(lua_State* L)
{
	lua_pushboolean(L, lua_isboolean(L, 1));
	return 1;
}

int is_userdata(lua_State* L)
{
	lua_pushboolean(L, lua_isuserdata(L, 1));
	return 1;
}

int is_nil(lua_State* L)
{
	lua_pushboolean(L, lua_isnil(L, 1));
	return 1;
}
/* -----------------------------------*/

const struct luaL_Reg functions_global[] = {
	{"toboolean", to_bool},
	{"is_string", is_string},
	{"is_number", is_number},
	{"is_bool", is_bool},
	{"is_function", is_function},
	{"is_table", is_table},
	{"is_userdata", is_userdata},
	{"is_nil", is_nil},
	{NULL, NULL}
};

int open_functions_global(lua_State* L)
{
	luaL_register(L, LUA_GLOBAL, functions_global);
	set_info_extend_lua(L);
	return 1;
}