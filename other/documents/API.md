# API Reference — SDK module

Complete description of the module's public API: registration macros,
argument types, result types and helper functions.

## Macros

### MTA_LUA_FUNCTION

```cpp
MTA_LUA_FUNCTION("name", "description")
{
    // body: lua_State* L is available; return the number of results
}
```

Body style is the primary one. Inside, lua_State *L is available. Exceptions
thrown in the body are caught by the framework and turned into Lua errors.
Return is a plain return <result count>; (how many values were pushed).

### MTA_LUA_FUNC

```cpp
MTA_LUA_FUNC("name", "description", function-or-lambda);
```

Lambda style for short functions. Parameter and return types are read from
the signature (see [Parameter types (lambda style)](#parameter-types-lambda-style));
the result is returned automatically.

## Reading arguments

### mta::lua::args

```cpp
auto [a, b] = mta::lua::args<double, double>(L);
```

Reads arguments 1..N from the stack, by the types in the template list, and
returns a std::tuple (for structured bindings). Every type is checked
automatically: on mismatch the error has the plan §7 format
`bad argument #N to '<function>' (expected <type>, got <actual>)`.
Extra arguments are ignored; missing ones give
`(expected <type>, got no value)` at the missing position.

### Argument types

| Type in args<...> | From Lua | Notes |
|---|---|---|
| double, float, lua_Number | number | |
| int, std::int64_t, … | integer | range-checked |
| bool | boolean | |
| std::string | string | numbers convert like in Lua |
| std::string_view | string | no copy, valid until the call ends |
| mta::lua::Argument | any value | tables read recursively |
| mta::lua::Table | table | non-table -> error |
| mta::async::Callback | function | stable reference |
| std::optional<T> | T or nil/nothing | nil -> nullopt |

### Parameter types (lambda style)

In addition to the list above, the lambda signature supports:

| Parameter type | Meaning |
|---|---|
| C++ default = value | omitted argument -> default |
| mta::lua::rest_args | trailing (variadic) arguments, last only |
| mta::lua::context | VM + resource name; consumes no Lua argument |

## Result types

mta::lua::push_results(L, ...) accepts values and returns their count:

| Result | In Lua |
|---|---|
| a number / string / bool / nullptr | one value (nullptr -> nil) |
| several values separated by commas | several results |
| mta::lua::Argument | one value (including tables) |
| mta::lua::Table | one table |
| mta::lua::Arguments (via .push(L)) | a whole result list |

## mta::lua::Argument

A snapshot of one Lua value. Accepts any value; tables are read recursively
up to mta::lua::max_table_depth (= 32) — protection against cyclic
references.

```cpp
enum class Type : int { None, Nil, Boolean, LightUserData, Number, String, Table };

Argument();                                   // None
Argument(std::nullptr_t);                     // Nil
Argument(bool);
Argument(lua_Number);
Argument(const char*);
Argument(std::string);
Argument(void*);                              // light userdata
Argument(Table);                              // table

Type type() const;                            // current type
bool as_boolean(bool def = false) const;
lua_Number as_number(lua_Number def = 0.0) const;
const std::string& as_string() const;
void* as_light_userdata() const;
bool is_table() const;
const Table& as_table() const;                // throws if not a table

void read(lua_State* L, int index, int depth = 0);  // read from the stack
void push(lua_State* L, int depth = 0) const;        // push onto the stack

operator== / operator!=;                      // deep comparison
```

## mta::lua::Table

A table snapshot: the integer sequence part + string fields.

```cpp
struct Table
{
    std::vector<Argument> array;                       // [1], [2], [3]…
    std::vector<std::pair<std::string, Argument>> fields;  // name = value
};
```

Keys of other types (boolean, tables, ...) are discarded when reading. Holes
in the sequence are filled with nil.

## mta::lua::Arguments

A flat value list — for marshaling argument sets (tables included).

```cpp
void read(lua_State* L, int index_begin = 1);   // read every argument
int push(lua_State* L) const;                   // push all, return the count
void append(const Arguments& other);
bool call(lua_State* L, const char* global_name, std::string* error_out = nullptr) const;
const Argument& at(std::size_t index) const;
std::size_t count() const;
bool empty() const;

Argument& push_nil();
Argument& push_boolean(bool);
Argument& push_number(lua_Number);
Argument& push_string(const char* / std::string);
Argument& push_light_userdata(void*);
```

## mta::lua::context

```cpp
struct context
{
    lua_State *vm;        // VM of the calling resource
    std::string resource; // resource name
};
```

## mta::lua::rest_args

```cpp
struct rest_args
{
    Arguments values;     // all trailing arguments
};
```

## Errors

```cpp
[[noreturn]] void raise(std::string message);        // throw -> Lua error
template<typename... A> [[noreturn]] void raise_error(A&&... args);  // streamed via <<
```

Any C++ exception inside a module function is turned into a Lua error by the
mta::lua::protected_call trampoline. The server process is protected from
exceptions.

Unified error model (plan §19) — mta::errors::Error carries a category:
Generic (deliberate module error, rendered verbatim), InvalidArgument,
InvalidType, MissingArgument, ResourceStopped, InvalidCallback,
InvalidObject, AsyncCancelled, InternalError. The InternalError category and
any non-mta exception render as `internal module error: ...`, so a framework
bug never looks like a scripter mistake. Categories: see sdk/errors/errors.hpp.

## mta::async::Callback

A stable reference to a Lua function that survives resource restarts.
Move-only.

```cpp
static Callback from_stack(lua_State* L, int index); // bind a function (throws on non-function)
bool valid() const;
const std::string& resource() const;                 // owning resource name
bool call(const mta::lua::Arguments&) const;         // call; false if the resource is dead/error
```

## mta::async::Scheduler

Internal task engine: background work with results delivered on the main
thread (DoPulse). Lua is never called from a worker thread.

```cpp
static Scheduler& instance();
void start();      // spawn workers ([async] workers in config/module.toml)
void stop();       // stop workers, cancel queued tasks (shutdown)
void pump();       // main thread: dispatch results, fire timers

[[nodiscard]] Task post_task(std::function<Arguments()> work,
                std::function<void(const Arguments&, const char*)> completion,
                std::string resource = {}, std::uint64_t generation = 0);
// work — on a worker (NO Lua!), completion — on the main thread;
// error == nullptr on success. The task is owned by (resource, generation)
// when a resource is given. Invalid handle when the queue is full.

std::uint64_t post_timer(std::string resource, int delay_ms, int repeat_count,
                         std::function<void(std::uint64_t)> completion);
// completion(tick) every delay_ms, repeat_count times (0 = forever).

bool cancel_timer(std::uint64_t id);
void handle_resource_stopped(const std::string& resource);
void configure(std::size_t queue_limit);  // runtime override of [async] queue
bool running() const;
```

## mta::async::run / Task

Developer-facing task API (plan §13): background work with a cancellable
handle and automatic resource ownership (plan §14).

```cpp
[[nodiscard]] Task run(lua_State* L,
                       std::function<Arguments()> work,
                       std::function<void(const Arguments&, const char*)> completion);

class Task {
    bool cancel();   // queued: never runs; running: result delivery suppressed
    bool done();     // nothing more will happen (done/cancelled)
    bool valid();    // false for a default handle / queue-full rejection
    std::uint64_t id();
};
```

The task is owned by the calling resource: when the resource stops, queued
tasks are cancelled and completions of the finished generation are dropped
before any Lua access. Queue limits come from `[async] queue`; worker count
from `[async] workers` (`"auto"` = hardware probe, clamped to 1..8).

## mta::timer

Developer-facing timers (plan §15): one-shot and repeating, with a handle
and automatic resource ownership.

```cpp
[[nodiscard]] Timer after(lua_State* L, int delay_ms, std::function<void()> fn);
// fires once on the main thread after delay_ms
[[nodiscard]] Timer every(lua_State* L, int delay_ms, std::function<void()> fn);
// repeats every delay_ms until cancelled or the resource stops

class Timer {
    bool cancel();  // true if a scheduled timer was cancelled
    bool valid();   // still scheduled (will fire again)
    std::uint64_t id();
};
```

Timers are resource-aware: they belong to the calling resource and its VM
generation. When the resource stops, every owned timer is invalidated, and
a restart of the same resource never revives one of an older generation.

## mta::resources::Store<T>

Per-resource data with automatic cleanup when the resource stops.

```cpp
template<typename T> class Store
{
    T& for_state(lua_State* L);               // data of the calling resource
    T* try_find(const std::string& resource); // or nullptr
    // on_resource_stopped / on_all_released — cleanup is automatic
};
```

## mta::log

Levels (ascending): Debug < Info < Warn < Error < Off. A message is printed
when its level >= the current one. Default is Info.

```cpp
enum class Level { Debug, Info, Warn, Error, Off };
void set_level(Level);
Level get_level();

template<typename... A> void debug(lua_State*, A&&...);  // bound to the resource
template<typename... A> void info(A&&...);    // server console
template<typename... A> void warn(A&&...);    // server console (warning)
template<typename... A> void error(A&&...);   // server console (error)
```

## mta::events

MTA event trigger: the module "throws" an event into the resource's Lua
scripts via the standard triggerEvent (source is root).

```cpp
bool trigger(lua_State* L, const char* event_name, const mta::lua::Arguments& args);
// false if triggerEvent is unavailable or the call failed.
```

## mta::lua — table helpers

```cpp
// Argument -> C++-type conversion (throws on mismatch).
template<typename T> T convert(const Argument&);

// Read a field by string key.
template<typename T> T get_field(const Table&, const char* key, T default_value);
template<typename T> T get_field(const Table&, const char* key);  // throws if absent

// Write (or overwrite) a field.
void set_field(Table&, const char* key, Argument value);
```

## mta::userdata::Registry<T>

Objects with methods and a __gc (destructor).

```cpp
template<typename T> class Registry
{
    using Registrar = void (*)(lua_State*);
    static void set_methods(Registrar);   // once per process
    static void ensure(lua_State*);       // metatable + methods in this VM
    static T* create(lua_State*, T value); // userdata on the stack, returns T*
    static T* check(lua_State*, int index); // validate userdata (throws)
    template<std::size_t Tag, typename F>
    static void add_method(lua_State*, const char* name, F fn);
};

// Register a method: MTA_METHOD(Type, "name", lambda);
// The lambda takes self (Type&) as its first parameter.
```

## mta::module

```cpp
struct Info { const char* name; const char* author; float version; };
Info info();
ILuaModuleManager10* manager();
std::string current_resource_name(lua_State* L);
```

## mta::registry::Registry

```cpp
static Registry& instance();
void add(Spec spec);
void register_all(ILuaModuleManager10&, lua_State*) const;
const std::vector<Spec>& functions() const;
std::size_t size() const;
```
