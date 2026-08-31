#pragma once

// Жизненный цикл модуля. Точки входа MTAEXPORT из module.cpp перенаправляют
// сюда; эти же хуки используют внутренние подсистемы.

#include <string>

struct ILuaModuleManager10;
struct lua_State;

namespace mta::module
{
struct Info
{
    const char *name;
    const char *author;
    float version;
};

Info info() noexcept;
ILuaModuleManager10 *manager() noexcept;

// Имя ресурса, которому принадлежит lua_vm; пустая строка, если не удалось
// определить. Использует ABI-безопасный char*-вариант менеджера (перегрузка
// со std::string пересекает границу DLL с чувствительной к ABI строкой).
std::string current_resource_name(lua_State *lua_vm) noexcept;

bool initialize(ILuaModuleManager10 *manager, char *module_name, char *author, float *version) noexcept;
void register_functions(lua_State *lua_vm) noexcept;
bool pulse() noexcept;
bool shutdown() noexcept;
bool resource_stopping(lua_State *lua_vm) noexcept;
bool resource_stopped(lua_State *lua_vm) noexcept;
} // namespace mta::module
