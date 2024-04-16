#include "bit.h"

int ROL(int a, int n)
{
	int t1, t2;
	n = n % (sizeof(a) * 8);
	t1 = a << n;
	t2 = a >> (sizeof(a) * 8 - n);
	return t1 | t2;
}
int ROR(int a, int n)
{
	int t1, t2;
	n = n % (sizeof(a) * 8);
	t1 = a >> n;
	t2 = a << (sizeof(a) * 8 - n);
	return t1 | t2;
}

int bit_tobit(lua_State* L)
{
	LUA_INTEGER n = luaL_checkinteger(L, 1);
	BYTE len = sizeof(n) * 8;
	char* s = new char[len + 1];
	_itoa(n, s, 2);
	lua_pushfstring(L, s);
	return 1;
}

int bit_tohex(lua_State* L)
{
	LUA_INTEGER n = luaL_checkinteger(L, 1);
	BYTE len = sizeof(n) * 2;
	char* s = new char[len + 1];
	_itoa(n, s, 16);
	lua_pushfstring(L, s);
	return 1;
}

int bit_not(lua_State* L)
{
	LUA_INTEGER n = luaL_checkinteger(L, 1);
	lua_pushinteger(L, ~n);
	return 1;
}

int bit_and(lua_State* L)
{
	LUA_INTEGER a = luaL_checkinteger(L, 1);
	LUA_INTEGER b = luaL_checkinteger(L, 2);
	lua_pushinteger(L, a & b);
	return 1;
}

int bit_or(lua_State* L)
{
	LUA_INTEGER a = luaL_checkinteger(L, 1);
	LUA_INTEGER b = luaL_checkinteger(L, 2);
	lua_pushinteger(L, a | b);
	return 1;
}

int bit_xor(lua_State* L)
{
	LUA_INTEGER a = luaL_checkinteger(L, 1);
	LUA_INTEGER b = luaL_checkinteger(L, 2);
	lua_pushinteger(L, a ^ b);
	return 1;
}

int bit_rol(lua_State* L)
{
	LUA_INTEGER a = luaL_checkinteger(L, 1);
	LUA_INTEGER n = luaL_checkinteger(L, 2);
	lua_pushinteger(L, ROL(a, n));
	return 1;
}

int bit_ror(lua_State* L)
{
	LUA_INTEGER a = luaL_checkinteger(L, 1);
	LUA_INTEGER n = luaL_checkinteger(L, 2);
	lua_pushinteger(L, ROR(a, n));
	return 1;
}

int bit_lshift(lua_State* L)
{
	LUA_INTEGER a = luaL_checkinteger(L, 1);
	LUA_INTEGER n = luaL_checkinteger(L, 2);
	lua_pushinteger(L, a << n);
	return 1;
}

int bit_rshift(lua_State* L)
{
	LUA_INTEGER a = luaL_checkinteger(L, 1);
	LUA_INTEGER n = luaL_checkinteger(L, 2);
	lua_pushinteger(L, a >> n);
	return 1;
}

const struct luaL_Reg bit_funcs[] = {
	{"tobit", bit_tobit},
	{"tohex", bit_tohex},
	{"bnot", bit_not},
	{"band", bit_and},
	{"bor", bit_or},
	{"bxor", bit_xor},
	{"lshift", bit_lshift},
	{"rshift", bit_rshift},
	{"rol", bit_rol},
	{"ror", bit_ror},
	{NULL, NULL}
};

int open_bit(lua_State* L)
{
	luaL_register(L, "bit", bit_funcs);
	return 1;
}