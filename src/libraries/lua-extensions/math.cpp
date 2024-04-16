#include <random>

#include "lua.hpp"
#include "math.h"

void set_info_extend_math(lua_State* L)
{
    lua_pushliteral(L, "EXTENDED_MATH_COPYRIGHT");
    lua_pushliteral(L, "Copyright (C) A.R.E.A 2017-2023.");
    lua_settable(L, -3);
    lua_pushliteral(L, "EXTENDED_MATH_DESCRIPTION");
    lua_pushliteral(L, "EXTENDED_MATH is a Lua library that extends and adds new methods to work with the math object luajit v2.1.");
    lua_settable(L, -3);
    lua_pushliteral(L, "EXTENDED_MATH_VERSION");
    lua_pushliteral(L, "EXTENDED_MATH v0.1.1");
    lua_settable(L, -3);
}

std::random_device ndrng;
std::mt19937 intgen;
std::uniform_real<float> float_random_01;

int gen_random_in_range(int a1, int a2)
{	//unsigned?
    static int cache_random{ 0 };
    std::uniform_int<> dist(a1, a2);
    intgen.seed(ndrng());
    int result = dist(intgen);

    if (cache_random == result)
    {
        result++;
        cache_random++;
        if (result > a2)
        {
            result = a1;
            cache_random = a1;
        }
    }
    else
        cache_random = result;

    return result;
}

int math_randomseed(lua_State* L)
{
    switch (lua_gettop(L))
    {
    case 0: {
        intgen.seed(ndrng());
        break;
    }
    case 1: {
        DWORD seed_value = luaL_checkint(L, 1);
        intgen.seed(seed_value);
        break;
    }
    default: return luaL_error(L, "math_randomseed: wrong number of arguments");
    }
    return 0;
}

int math_random(lua_State* L)
{
    switch (lua_gettop(L))
    {
    case 0: {
        lua_pushnumber(L, float_random_01(intgen));
        break;
    }
    case 1: {
        //luaL_argcheck(L, 1 <= u, 1, "interval is empty");
        luaL_checktype(L, 1, LUA_TNUMBER);
        lua_pushinteger(L, gen_random_in_range(1, luaL_checkint(L, 1)));
        break;
    }
    case 2: {
        //luaL_argcheck(L, l <= u, 2, "interval is empty");
        luaL_checktype(L, 1, LUA_TNUMBER);
        luaL_checktype(L, 2, LUA_TNUMBER);
        lua_pushinteger(L, gen_random_in_range(luaL_checkint(L, 1), luaL_checkint(L, 2)));
        break;
    }
    default: return luaL_error(L, "wrong number of arguments");
    }
    return 1;
}

int math_clamp(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TNUMBER);
    luaL_checktype(L, 2, LUA_TNUMBER);
    luaL_checktype(L, 3, LUA_TNUMBER);
    lua_Number value = lua_tonumber(L, 1);
    lua_Number min = lua_tonumber(L, 2);
    lua_Number max = lua_tonumber(L, 3);
    lua_pushnumber(L, std::clamp(value, min, max));
    return 1;
}

const struct luaL_Reg mathlib[] = {
    {"random", math_random},
    {"randomseed", math_randomseed},
    {"clamp", math_clamp},
    {NULL, NULL}
};

int open_math(lua_State* L)
{
    luaL_register(L, LUA_MATHLIBNAME, mathlib);
    set_info_extend_math(L);
    return 1;
}