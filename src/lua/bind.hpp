#pragma once

// Typed binder for Lua functions -- the module's main API.
//
//     MTA_LUA_FUNC("my_sum", "Adds two numbers.",
//         [](double a, double b) { return a + b; });
//
// Parameter types are read straight from the signature: the framework checks
// arguments itself, produces a readable Lua error on a type mismatch and
// returns the result to Lua automatically. No indices, check_* or manual
// push_results are needed.
//
// Supported parameter types:
//   double / float          number
//   bool                    boolean
//   int / int64_t / ...     integer (range-checked)
//   std::string             string
//   std::string_view        string without copying (valid until call end)
//   mta::lua::Argument      any value (tables are read recursively)
//   mta::lua::Table         table (error if a non-table was passed)
//   mta::async::Callback    Lua function (stable reference to it)
//   std::optional<T>        optional argument: nil/absent -> nullopt
//   mta::lua::rest_args     trailing (variadic) arguments, last parameter only
//   mta::lua::context       VM and resource name; takes NO Lua argument
//
// Optional parameters may also be written with plain C++ defaults:
//
//     MTA_LUA_FUNC("my_greet", "Greets a name.",
//         [](std::string name, std::string greeting = "hi") {
//             return greeting + ", " + name;
//         });
//
// Result types: any value-parameter type, plus
//   void                       return nothing
//   std::tuple / std::pair     several results
//   std::vector<T>             elements are expanded into a result list
//   std::optional<T>           nil when the value is absent
//   mta::lua::Arguments        a whole result list

#include "lua/argument.hpp"
#include "lua/arguments.hpp"
#include "lua/protect.hpp"
#include "lua/stack.hpp"
#include "module/module.hpp"
#include "runtime/callback.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace mta::lua
{
// Trailing (variadic) arguments: the function's last parameter catches
// everything passed beyond the fixed arguments.
struct rest_args
{
    Arguments values;
};

// Call context: VM and the name of the calling resource. A parameter of this
// type consumes no Lua argument and may appear anywhere in the signature.
struct context
{
    lua_State *vm = nullptr;
    std::string resource;
};

namespace detail
{
// --- callable signature -------------------------------------------------------

template <typename R, typename... Args>
struct callable_traits_base
{
    using result = R;
    using args = std::tuple<Args...>;
    static constexpr std::size_t arity = sizeof...(Args);
};

template <typename T>
struct callable_traits;

template <typename R, typename... Args>
struct callable_traits<R (*)(Args...)> : callable_traits_base<R, Args...>
{
};

template <typename R, typename... Args>
struct callable_traits<R (&)(Args...)> : callable_traits_base<R, Args...>
{
};

template <typename F>
struct callable_traits : callable_traits<decltype(&F::operator())>
{
};

template <typename F, typename R, typename... Args>
struct callable_traits<R (F::*)(Args...) const> : callable_traits_base<R, Args...>
{
};

template <typename F, typename R, typename... Args>
struct callable_traits<R (F::*)(Args...)> : callable_traits_base<R, Args...>
{
};

template <typename F, typename R, typename... Args>
struct callable_traits<R (F::*)(Args...) const noexcept> : callable_traits_base<R, Args...>
{
};

template <typename F, typename R, typename... Args>
struct callable_traits<R (F::*)(Args...) noexcept> : callable_traits_base<R, Args...>
{
};

// --- parameter classification --------------------------------------------------

template <typename T>
inline constexpr bool is_optional_v = false;
template <typename T>
inline constexpr bool is_optional_v<std::optional<T>> = true;

template <typename T>
inline constexpr bool is_rest_v = false;
template <>
inline constexpr bool is_rest_v<rest_args> = true;

// A parameter that requires an explicit Lua argument: not optional, not rest
// and not context.
template <typename T>
inline constexpr bool is_required_param_v =
    !is_optional_v<T> && !is_rest_v<T> && !std::is_same_v<T, context>;

// --- reading a single argument --------------------------------------------------

template <typename T>
T pull_arg(lua_State *L, int index)
{
    using U = std::remove_cvref_t<T>;

    if constexpr (std::is_same_v<U, Argument>)
    {
        Argument value;
        value.read(L, index);
        return value;
    }
    else if constexpr (std::is_same_v<U, Table>)
    {
        const int normalized = normalize_index(L, index);
        Argument value;
        value.read(L, normalized);
        if (!value.is_table())
        {
            raise_error("argument #", index, " must be a table, got ",
                        type_name(lua_type(L, normalized)));
        }
        return std::move(value.as_table());
    }
    else if constexpr (std::is_same_v<U, mta::async::Callback>)
    {
        return mta::async::Callback::from_stack(L, index);
    }
    else if constexpr (is_optional_v<U>)
    {
        using inner_type = typename U::value_type;
        const int type_value = lua_type(L, normalize_index(L, index));
        if (type_value == LUA_TNONE || type_value == LUA_TNIL)
        {
            return U{};
        }
        return U{pull_arg<inner_type>(L, index)};
    }
    else if constexpr (std::is_same_v<U, rest_args>)
    {
        rest_args rest;
        rest.values.read(L, index);
        return rest;
    }
    else if constexpr (std::is_same_v<U, bool>)
    {
        return check_boolean(L, index);
    }
    else if constexpr (std::is_same_v<U, std::string>)
    {
        return check_string(L, index);
    }
    else if constexpr (std::is_same_v<U, std::string_view>)
    {
        // The string lives in Lua until the call ends; no copy is needed.
        const int normalized = normalize_index(L, index);
        if (lua_isstring(L, normalized) == 0)
        {
            raise_error("argument #", index, " must be a string, got ",
                        type_name(lua_type(L, normalized)));
        }
        std::size_t length = 0;
        const char *text = lua_tolstring(L, normalized, &length);
        return std::string_view{text ? text : "", length};
    }
    else if constexpr (std::is_same_v<U, float>)
    {
        return static_cast<float>(check_number(L, index));
    }
    else if constexpr (std::is_same_v<U, double> || std::is_same_v<U, lua_Number>)
    {
        return static_cast<U>(check_number(L, index));
    }
    else if constexpr (std::is_integral_v<U> && !std::is_same_v<U, bool> && !std::is_same_v<U, char> &&
                       !std::is_same_v<U, wchar_t>)
    {
        const lua_Integer value = check_integer(L, index);
        if constexpr (std::is_unsigned_v<U>)
        {
            if (value < 0 || static_cast<std::uint64_t>(value) >
                                 static_cast<std::uint64_t>(std::numeric_limits<U>::max()))
            {
                raise_error("argument #", index, " is out of range");
            }
        }
        else
        {
            if (value < static_cast<lua_Integer>(std::numeric_limits<U>::min()) ||
                value > static_cast<lua_Integer>(std::numeric_limits<U>::max()))
            {
                raise_error("argument #", index, " is out of range");
            }
        }
        return static_cast<U>(value);
    }
    else
    {
        static_assert(!sizeof(U), "unsupported parameter type (see lua/bind.hpp)");
    }
}

// Reads a parameter, honoring context (which consumes no Lua argument).
template <typename T>
T pull_param(lua_State *L, std::size_t &index)
{
    using U = std::remove_cvref_t<T>;

    if constexpr (std::is_same_v<U, context>)
    {
        context ctx;
        ctx.vm = L;
        ctx.resource = mta::module::current_resource_name(L);
        return ctx;
    }
    else
    {
        return pull_arg<U>(L, static_cast<int>(index++));
    }
}

// --- result layout --------------------------------------------------------------

template <typename T>
struct is_tuple_like : std::false_type
{
};
template <typename... Ts>
struct is_tuple_like<std::tuple<Ts...>> : std::true_type
{
};
template <typename A, typename B>
struct is_tuple_like<std::pair<A, B>> : std::true_type
{
};

template <typename T>
struct is_vector : std::false_type
{
};
template <typename T, typename A>
struct is_vector<std::vector<T, A>> : std::true_type
{
};

template <typename T>
int push_result(lua_State *L, T &&value)
{
    using U = std::remove_cvref_t<T>;

    if constexpr (is_optional_v<U>)
    {
        if (!value.has_value())
        {
            lua_pushnil(L);
            return 1;
        }
        return push_result(L, *value);
    }
    else if constexpr (is_tuple_like<U>::value)
    {
        return std::apply(
            [L](auto &&...elements) {
                return (push_result(L, std::forward<decltype(elements)>(elements)) + ... + 0);
            },
            std::forward<T>(value));
    }
    else if constexpr (is_vector<U>::value)
    {
        int pushed = 0;
        for (auto &element : value)
        {
            pushed += push_result(L, element);
        }
        return pushed;
    }
    else if constexpr (std::is_same_v<U, Arguments>)
    {
        // Arguments is a RESULT LIST: push all of them and return the count.
        return value.push(L);
    }
    else
    {
        push_one(L, std::forward<T>(value));
        return 1;
    }
}

// --- arity and invocation ----------------------------------------------------------

// Closure storage; defined below.
template <std::size_t Tag, typename F>
struct holder;

// Calls f with the first K parameters; missing ones are synthesized
// (optional -> nullopt, rest -> empty); C++ defaults are applied by the
// compiler itself.
template <std::size_t Tag, typename F, std::size_t K>
int invoke_prefix(lua_State *L)
{
    using args_type = typename callable_traits<F>::args;
    using result_type = typename callable_traits<F>::result;

    std::size_t next_index = 1;
    auto args = [&next_index, L]<std::size_t... I>(std::index_sequence<I...>) {
        return std::tuple{pull_param<std::tuple_element_t<I, args_type>>(L, next_index)...};
    }(std::make_index_sequence<K>{});

    if constexpr (std::is_void_v<result_type>)
    {
        std::apply(holder<Tag, F>::stored, std::move(args));
        return 0;
    }
    else
    {
        return push_result(L, std::apply(holder<Tag, F>::stored, std::move(args)));
    }
}

// Can f be invoked with the first J parameters (C++ defaults included)?
template <typename F, std::size_t J, std::size_t... I>
constexpr bool prefix_invocable_impl(std::index_sequence<I...>)
{
    using args_type = typename callable_traits<F>::args;
    return std::is_invocable_v<F, std::tuple_element_t<I, args_type>...>;
}

template <typename F, std::size_t J>
constexpr bool prefix_invocable_v = prefix_invocable_impl<F, J>(std::make_index_sequence<J>{});

// required_counts[J] -- how many of the first J parameters require an explicit
// Lua argument (not optional, not rest, not context).
template <typename F, std::size_t... I>
constexpr std::array<std::size_t, sizeof...(I) + 1> required_counts_impl(std::index_sequence<I...>)
{
    using args_type = typename callable_traits<F>::args;
    std::array<std::size_t, sizeof...(I) + 1> counts{};
    std::size_t running = 0;
    ((counts[I] = running,
      running +=
          (is_required_param_v<std::remove_cvref_t<std::tuple_element_t<I, args_type>>> ? 1u : 0u)),
     ...);
    counts[sizeof...(I)] = running;
    return counts;
}

struct dispatch_entry
{
    int (*invoke)(lua_State *);
    bool invocable;
};

template <std::size_t Tag, typename F, std::size_t J>
struct dispatch_slot
{
    static constexpr bool invocable = prefix_invocable_v<F, J>;

    static int invoke(lua_State *L)
    {
        if constexpr (invocable)
        {
            return invoke_prefix<Tag, F, J>(L);
        }
        else
        {
            // Unreachable: slots without invocable are never selected.
            raise_error("argument #", J + 1, " is missing");
        }
    }
};

template <std::size_t Tag, typename F, std::size_t... Js>
constexpr std::array<dispatch_entry, sizeof...(Js)> make_dispatch_table_impl(
    std::index_sequence<Js...>)
{
    return std::array<dispatch_entry, sizeof...(Js)>{
        dispatch_entry{&dispatch_slot<Tag, F, Js>::invoke, dispatch_slot<Tag, F, Js>::invocable}...};
}

// Function storage: a static copy of the closure plus the entry point for Lua.
template <std::size_t Tag, typename F>
struct holder
{
    static inline F stored{};

    using traits = callable_traits<F>;
    static constexpr std::size_t arity = traits::arity;

    static constexpr auto table =
        make_dispatch_table_impl<Tag, F>(std::make_index_sequence<arity + 1>{});
    static constexpr auto required_counts =
        required_counts_impl<F>(std::make_index_sequence<arity>{});

    static int entry(lua_State *L) noexcept
    {
        try
        {
            const std::size_t k = static_cast<std::size_t>(lua_gettop(L));
            for (std::size_t J = arity + 1; J-- > 0;)
            {
                if (table[J].invocable && required_counts[J] <= k)
                {
                    return table[J].invoke(L);
                }
            }
            // No arity matched: the first required parameter will produce a
            // typed "got no value"/"got nil" error.
            return error_probe(L);
        }
        catch (const std::exception &e)
        {
            return luaL_error(L, "%s", e.what());
        }
        catch (...)
        {
            return luaL_error(L, "unknown C++ exception in module function");
        }
    }

    // Reads every parameter just to produce a readable error; synthesized
    // parameters are read silently.
    static int error_probe(lua_State *L)
    {
        using args_type = typename traits::args;
        std::size_t index = 1;
        [&index, L]<std::size_t... I>(std::index_sequence<I...>) {
            ((void)pull_param<std::remove_cvref_t<std::tuple_element_t<I, args_type>>>(L, index),
             ...);
        }(std::make_index_sequence<arity>{});
        raise_error("missing required arguments");
    }
};

// rest_args may only be the last parameter.
template <typename F, std::size_t... I>
constexpr bool rest_only_last_impl(std::index_sequence<I...>)
{
    using args_type = typename callable_traits<F>::args;
    constexpr std::size_t n = sizeof...(I);
    bool ok = true;
    ((ok = ok && (I + 1 == n || !is_rest_v<std::remove_cvref_t<std::tuple_element_t<I, args_type>>>)),
     ...);
    return ok;
}

template <typename F>
constexpr bool rest_only_last_v = rest_only_last_impl<F>(
    std::make_index_sequence<callable_traits<F>::arity>{});

// Adds a function to the registry (implemented in registry/registry.cpp).
bool register_function(const char *name, const char *description, int (*entry)(lua_State *));

// Registers a typed function; Tag is a unique number per call site.
template <std::size_t Tag, typename F>
bool register_typed(const char *name, const char *description, F function)
{
    using G = std::decay_t<F>;

    static_assert(rest_only_last_v<G>, "rest_args may only be the last parameter");

    holder<Tag, G>::stored = std::move(function);
    return register_function(name, description, &holder<Tag, G>::entry);
}
} // namespace detail

// Reads typed arguments (1..N) as a tuple -- for structured bindings:
//
//     auto [a, b] = mta::lua::args<double, double>(L);
//
// Types are validated automatically; optional<T> accepts nil/absence.
// Extra arguments are ignored; missing ones produce a readable Lua error.
template <typename... Ts>
std::tuple<Ts...> args(lua_State *L)
{
    std::size_t index = 1;
    return std::tuple<Ts...>{detail::pull_param<Ts>(L, index)...};
}
} // namespace mta::lua
