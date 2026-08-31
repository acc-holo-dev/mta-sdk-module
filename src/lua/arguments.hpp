#pragma once

// Плоский список значений Lua (включая таблицы). Используется для
// маршалинга наборов аргументов между Lua и C++, например чтобы
// передать результат фоновой задачи в callback.

#include "lua/argument.hpp"

#include <string>
#include <vector>

namespace mta::lua
{
class Arguments
{
public:
    Arguments() = default;

    // Читает все аргументы со стека, начиная с index_begin (по умолчанию 1).
    void read(lua_State *lua_vm, int index_begin = 1);
    // Кладёт все значения на стек, возвращает количество.
    int push(lua_State *lua_vm) const;
    void append(const Arguments &other);

    // Вызывает глобальную Lua-функцию по имени с этими аргументами
    // (через pcall). Возвращает false и заполняет error_out (если задан)
    // при неудаче.
    bool call(lua_State *lua_vm, const char *function_name, std::string *error_out = nullptr) const;

    Argument &push_nil();
    Argument &push_boolean(bool value);
    Argument &push_number(lua_Number value);
    Argument &push_string(const char *value);
    Argument &push_string(std::string value);
    Argument &push_light_userdata(void *value);

    [[nodiscard]] std::size_t count() const noexcept { return arguments_.size(); }
    [[nodiscard]] bool empty() const noexcept { return arguments_.empty(); }
    [[nodiscard]] const Argument &at(std::size_t index) const;

private:
    std::vector<Argument> arguments_{};
};
} // namespace mta::lua
