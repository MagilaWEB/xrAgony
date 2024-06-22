#include <random>

#include "table.h"
#include "math.h"

void set_info_extend_table(lua_State* L)
{
	lua_pushliteral(L, "EXTENDED_TABLE_COPYRIGHT");
	lua_pushliteral(L, "Copyright (C) MAGILA.");
	lua_settable(L, -3);
	lua_pushliteral(L, "EXTENDED_TABLE_DESCRIPTION");
	lua_pushliteral(L, "EXTENDED_TABLE is a Lua library that extends and adds new methods for working with table objects luajit v2.1.");
	lua_settable(L, -3);
	lua_pushliteral(L, "EXTENDED_TABLE_VERSION");
	lua_pushliteral(L, "EXTENDED_TABLE v0.1.2 (20.06.2024)");
	lua_settable(L, -3);
}

inline DWORD C_get_size(lua_State* L)
{
	int i = 0;
	lua_settop(L, 2);
	while (lua_next(L, 1))
	{
		++i;
		lua_pop(L, 1);
	}
	return i;
}

int tab_keys(lua_State* L)
{
	int i = 1;
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_newtable(L);
	lua_pushnil(L);
	while (lua_next(L, 1) != 0)
	{
		lua_pushinteger(L, i);
		++i;
		lua_pushvalue(L, -3);
		lua_settable(L, 2);
		lua_pop(L, 1);
	}
	return 1;
}

int tab_values(lua_State* L)
{
	int i = 1;
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_newtable(L);
	lua_pushnil(L);
	while (lua_next(L, 1) != 0)
	{
		lua_pushinteger(L, i);
		++i;
		lua_pushvalue(L, -2);
		lua_settable(L, 2);
		lua_pop(L, 1);
	}
	return 1;
}

int get_size(lua_State* L)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_pushinteger(L, C_get_size(L));
	return 1;
}

int get_empty(lua_State* L)
{
	if (lua_type(L, 1) == LUA_TNIL)
	{
		lua_pushboolean(L, 1);
		return 1;
	}

	luaL_checktype(L, 1, LUA_TTABLE);
	lua_settop(L, 2);
	while (lua_next(L, 1))
	{
		lua_pushboolean(L, 0);
		return 1;
	}

	lua_pushboolean(L, 1);
	return 1;
}

int get_random(lua_State* L)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	int i = C_get_size(L);
	if (i)
	{
		int j = gen_random_in_range(L, 1, i);
		i = 0;
		lua_settop(L, 2);
		while (lua_next(L, 1))
		{
			++i;
			if (i == j)
			{
				lua_pushvalue(L, -2);
				lua_pushvalue(L, -2);
				return 2;
			}
			lua_pop(L, 1);
		}
	}
	
	lua_pushnil(L);
	return 1;
}

int get_random_value(lua_State* L)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	int i = C_get_size(L);
	if (i)
	{
		int j = gen_random_in_range(L, 1, i);
		i = 0;
		lua_settop(L, 2);
		while (lua_next(L, 1))
		{
			++i;
			if (i == j)
			{
				lua_pushvalue(L, -1);
				lua_pushvalue(L, -1);
				return 2;
			}
			lua_pop(L, 1);
		}
	}
	lua_pushnil(L);
	return 1;
}

const struct luaL_Reg tab_funcs[] = {
	{"keys", tab_keys},
	{"values", tab_values},
	{"size", get_size},
	{"random", get_random},
	{"random_value", get_random_value},
	{"empty", get_empty},
	{NULL, NULL}
};

int open_table(lua_State* L)
{
	luaL_openlib(L, LUA_TABLIBNAME, tab_funcs, 0);
	set_info_extend_table(L);
	return 1;
}