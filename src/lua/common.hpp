#pragma once

// Единая точка подключения Lua 5.1 (вендоренные заголовки из vendor/mta-sdk).

extern "C"
{
#include "lua/lauxlib.h"
#include "lua/lualib.h"
#include "lua/lua.h"
}

#ifndef LUA_OK
#define LUA_OK 0
#endif
