#include "lua.hpp"
#include "string.h"
#include <cstdlib>
#include <cctype>

void set_info_extend_string(lua_State* L)
{
	lua_pushliteral(L, "EXTENDED_STRING_COPYRIGHT");
	lua_pushliteral(L, "Copyright (C) A.R.E.A 2017-2023.");
	lua_settable(L, -3);
	lua_pushliteral(L, "EXTENDED_STRING_DESCRIPTION");
	lua_pushliteral(L, "EXTENDED_STRING is a Lua library that extends and adds new methods for working with string objects luajit v2.1.");
	lua_settable(L, -3);
	lua_pushliteral(L, "EXTENDED_STRING_VERSION");
	lua_pushliteral(L, "EXTENDED_STRING v0.1.1 (21.11.2023)");
	lua_settable(L, -3);
}

/******************** STRING ********************/
int str_trim(lua_State* L)
{
	luaL_checktype(L, 1, LUA_TSTRING);
	const char* front;
	const char* end;
	size_t	  size;
	front = luaL_checklstring(L, 1, &size);
	end = &front[size - 1];
	for (; size && isspace(*front); size--, front++)
		;
	for (; size && isspace(*end); size--, end--)
		;
	lua_pushlstring(L, front, (size_t)(end - front) + 1);
	return 1;
}

int str_trim_l(lua_State* L)
{
	luaL_checktype(L, 1, LUA_TSTRING);
	const char* front;
	const char* end;
	size_t	  size;
	front = luaL_checklstring(L, 1, &size);
	end = &front[size - 1];
	for (; size && isspace(*front); size--, front++)
		;
	lua_pushlstring(L, front, (size_t)(end - front) + 1);
	return 1;
}

int str_trim_r(lua_State* L)
{
	luaL_checktype(L, 1, LUA_TSTRING);
	const char* front;
	const char* end;
	size_t	  size;
	front = luaL_checklstring(L, 1, &size);
	end = &front[size - 1];
	for (; size && isspace(*end); size--, end--)
		;
	lua_pushlstring(L, front, (size_t)(end - front) + 1);
	return 1;
}

int str_trim_w(lua_State* L)
{
	luaL_checktype(L, 1, LUA_TSTRING);
	int i = 0, d, n;
	const char* s = luaL_checkstring(L, 1);;
	while (s[i] == ' ') i++;
	n = i;
	while (s[i] != ' ' && s[i]) i++;
	d = i - n;
	lua_pushlstring(L, s + n, d);
	return 1;
}

const struct luaL_Reg strlib[] = {
	{"trim", str_trim},
	{"trim_l", str_trim_l},
	{"trim_r", str_trim_r},
	{"trim_w", str_trim_w},
	{NULL, NULL}
};

int open_string(lua_State* L)
{
	luaL_register(L, LUA_STRLIBNAME, strlib);
	set_info_extend_string(L);
	return 1;
}
/******************** STRING END ********************/