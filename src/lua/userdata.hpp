#pragma once

// userdata/metatables: objects with methods and automatic memory release.
//
// Lets you create Lua objects that have methods (obj:method(...)) and a
// destructor (__gc), like in full-featured libraries (sockets, mysql).
//
//     struct Counter { double value = 0; };
//
//     // Register the methods (once, lazily):
//     MTA_METHOD(Counter, "get", [](Counter &self) { return self.value; });
//     MTA_METHOD(Counter, "set", [](Counter &self, double v) { self.value = v; });
//
//     // Create an object from a module function:
//     MTA_LUA_FUNCTION("counter_create", "Creates a counter.")
//     {
//         auto [value] = mta::lua::args<double>(L);
//         mta::userdata::Registry<Counter>::create(L, Counter{value});
//         return 1;
//     }
//
//     -- Lua:
//     local c = counter_create(42)
//     c:get()   -- 42
//     c:set(100)
//     c = nil   -- __gc calls ~Counter()

#include "lua/bind.hpp"
#include "lua/protect.hpp"

#include <string>
#include <type_traits>
#include <typeinfo>
#include <utility>

namespace mta::userdata
{
template <typename T>
class Registry
{
public:
    // Function that registers the type's methods (called once per VM).
    using Registrar = void (*)(lua_State *);

    // Sets the method registrar. Call once (e.g. from a static initializer)
    // before the first create/check.
    static void set_methods(Registrar registrar)
    {
        registrar_() = registrar;
    }

    // Registers the metatable in THIS VM (lazily) and calls the registrar.
    static void ensure(lua_State *L)
    {
        luaL_getmetatable(L, type_name());
        if (!lua_isnil(L, -1))
        {
            lua_pop(L, 1);
            return;
        }
        lua_pop(L, 1);

        luaL_newmetatable(L, type_name());

        // __gc — the object destructor.
        lua_pushcfunction(L, &gc_metamethod);
        lua_setfield(L, -2, "__gc");

        // __index — the method table.
        lua_newtable(L);
        lua_setfield(L, -2, "__index");

        lua_pop(L, 1);

        // Methods are registered in this VM (every resource has its own
        // lua_State).
        if (registrar_() != nullptr)
        {
            registrar_()(L);
        }
    }

    // Creates T as userdata on the stack and attaches the metatable. Returns T*.
    static T *create(lua_State *L, T value)
    {
        ensure(L);
        void *memory = lua_newuserdata(L, sizeof(T));
        T *object = new (memory) T(std::move(value));
        luaL_getmetatable(L, type_name());
        lua_setmetatable(L, -2);
        return object;
    }

    // Validates userdata at the index; throws a readable error on a mismatch.
    static T *check(lua_State *L, int index)
    {
        if (lua_type(L, index) != LUA_TUSERDATA)
        {
            mta::lua::raise_error("argument #", index, " must be a module object, got ",
                                  mta::lua::detail::type_name(lua_type(L, index)));
        }

        if (lua_getmetatable(L, index) == 0)
        {
            mta::lua::raise_error("argument #", index, " is not a module object");
        }
        luaL_getmetatable(L, type_name());
        const bool matches = lua_rawequal(L, -1, -2) != 0;
        lua_pop(L, 2);

        if (!matches)
        {
            mta::lua::raise_error("argument #", index, " is not a module object");
        }

        return static_cast<T *>(lua_touserdata(L, index));
    }

    // Registers a method (obj:method(...)). Tag is unique per call site.
    template <std::size_t Tag, typename F>
    static void add_method(lua_State *L, const char *name, F fn)
    {
        ensure(L);
        method_holder<Tag, F>::fn = std::move(fn);

        luaL_getmetatable(L, type_name());
        lua_getfield(L, -1, "__index");
        // lua_pushcclosure directly: the lua_pushcfunction macro cannot cope
        // with a comma in template arguments like method_holder<Tag, F>.
        lua_pushcclosure(L, &method_holder<Tag, F>::trampoline, 0);
        lua_setfield(L, -2, name);
        lua_pop(L, 2);
    }

private:
    static Registrar &registrar_()
    {
        static Registrar registrar = nullptr;
        return registrar;
    }

    static const char *type_name()
    {
        static const std::string name = std::string("mta.userdata.") + typeid(T).name();
        return name.c_str();
    }

    static int gc_metamethod(lua_State *L)
    {
        T *object = static_cast<T *>(lua_touserdata(L, 1));
        object->~T();
        return 0;
    }

    template <std::size_t Tag, typename F>
    struct method_holder
    {
        static inline F fn{};

        static int trampoline(lua_State *L)
        {
            try
            {
                T *self = Registry<T>::check(L, 1);
                return invoke_method(L, self, fn);
            }
            catch (const std::exception &e)
            {
                return luaL_error(L, "%s", e.what());
            }
            catch (...)
            {
                return luaL_error(L, "unknown C++ exception in method");
            }
        }
    };

    // Calls fn(*self, args...), with args read from the stack starting at 2.
    template <typename F>
    static int invoke_method(lua_State *L, T *self, F &fn)
    {
        using traits = mta::lua::detail::callable_traits<F>;
        using args_type = typename traits::args;
        using result_type = typename traits::result;
        constexpr std::size_t arity = traits::arity;

        static_assert(arity >= 1, "a method must take self as its first parameter");

        std::size_t index = 2;
        auto args = [&index, L]<std::size_t... I>(std::index_sequence<I...>) {
            // I + 1 skips self (the first signature parameter).
            return std::tuple{
                mta::lua::detail::pull_param<std::tuple_element_t<I + 1, args_type>>(L, index)...};
        }(std::make_index_sequence<arity - 1>{});

        if constexpr (std::is_void_v<result_type>)
        {
            std::apply([&](auto &&...a) { fn(*self, std::forward<decltype(a)>(a)...); },
                       std::move(args));
            return 0;
        }
        else
        {
            return mta::lua::detail::push_result(
                L, std::apply([&](auto &&...a) { return fn(*self, std::forward<decltype(a)>(a)...); },
                              std::move(args)));
        }
    }
};
} // namespace mta::userdata

// Registers an object method: MTA_METHOD(Type, "name", lambda);
// The lambda takes self (Type&) as its first parameter.
#define MTA_METHOD(Type, Name, Fn)     ::mta::userdata::Registry<Type>::add_method<__COUNTER__>(L, (Name), (Fn))
