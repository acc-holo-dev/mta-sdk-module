# API-справочник — ml_base

Полное описание публичного API модуля: макросы регистрации, типы аргументов,
типы результатов и вспомогательные функции.

## Макросы

### MTA_LUA_FUNCTION

```cpp
MTA_LUA_FUNCTION("имя", "описание")
{
    // тело: доступен lua_State* L; вернуть количество результатов через return
}
```

Тело-стиль — основной. Внутри доступен `lua_State *L`. Исключения, брошенные
в теле, перехватываются каркасом и превращаются в Lua-ошибку. Возврат —
обычный `return <число результатов>;` (сколько значений положили на стек).

### MTA_LUA_FUNC

```cpp
MTA_LUA_FUNC("имя", "описание", функция-или-лямбда);
```

Лямбда-стиль для коротких функций. Типы параметров и возврата читаются из
сигнатуры (см. [Типы параметров](#типы-параметров-lambda-стиль)), результат
возвращается автоматически.

## Чтение аргументов

### mta::lua::args

```cpp
auto [a, b] = mta::lua::args<double, double>(L);
```

Читает аргументы 1..N со стека по типам из списка шаблона, возвращает
`std::tuple` (для structured bindings). Каждый тип проверяется автоматически:
при несовпадении бросается ошибка `argument #N must be <тип>, got <факт>`.
Лишние аргументы игнорируются, недостающие дают `…got no value`.

### Типы аргументов

| Тип в args<...> | Из Lua | Примечание |
|---|---|---|
| `double`, `float`, `lua_Number` | число | |
| `int`, `std::int64_t`, … | целое | с проверкой диапазона |
| `bool` | boolean | |
| `std::string` | строка | числа конвертируются, как в Lua |
| `std::string_view` | строка | без копирования, живёт до конца вызова |
| `mta::lua::Argument` | любое значение | таблицы читаются рекурсивно |
| `mta::lua::Table` | таблица | не-таблица → ошибка |
| `mta::async::Callback` | функция | стабильная ссылка |
| `std::optional<T>` | T или nil/ничего | nil → nullopt |

### Типы параметров (лямбда-стиль)

Дополнительно к списку выше, в сигнатуре лямбды доступны:

| Тип параметра | Смысл |
|---|---|
| C++-дефолт `= значение` | опущенный аргумент → дефолт |
| `mta::lua::rest_args` | хвостовые (вариадические) аргументы, только последним |
| `mta::lua::context` | VM + имя ресурса; аргумента в Lua не занимает |

## Типы результатов

`mta::lua::push_results(L, ...)` принимает значения и возвращает их число:

| Результат | В Lua |
|---|---|
| число / строка / bool / nullptr | одно значение (nullptr → nil) |
| несколько значений через запятую | несколько результатов |
| `mta::lua::Argument` | одно значение (в т.ч. таблица) |
| `mta::lua::Table` | одна таблица |
| `mta::lua::Arguments` (через `.push(L)`) | целый список результатов |

## mta::lua::Argument

Снимок одного значения Lua. Принимает любое значение; таблицы читаются
рекурсивно до глубины `mta::lua::max_table_depth` (= 32) — защита от
циклических ссылок.

```cpp
enum class Type : int { None, Nil, Boolean, LightUserData, Number, String, Table };

Argument();                                   // None
Argument(std::nullptr_t);                     // Nil
Argument(bool);
Argument(lua_Number);
Argument(const char*);
Argument(std::string);
Argument(void*);                              // light userdata
Argument(Table);                              // таблица

Type type() const;                            // текущий тип
bool as_boolean(bool def = false) const;
lua_Number as_number(lua_Number def = 0.0) const;
const std::string& as_string() const;
void* as_light_userdata() const;
bool is_table() const;
const Table& as_table() const;                // бросает, если не таблица

void read(lua_State* L, int index, int depth = 0);  // прочитать со стека
void push(lua_State* L, int depth = 0) const;        // положить на стек

operator== / operator!=;                      // глубокое сравнение
```

## mta::lua::Table

Снимок таблицы: целочисленная последовательная часть + строковые поля.

```cpp
struct Table
{
    std::vector<Argument> array;                       // [1], [2], [3]…
    std::vector<std::pair<std::string, Argument>> fields;  // name = value
};
```

Ключи других типов (boolean, таблицы и т.п.) при чтении отбрасываются.
Дыры в последовательности заполняются nil.

## mta::lua::Arguments

Плоский список значений — для маршалинга наборов аргументов (в т.ч. таблиц).

```cpp
void read(lua_State* L, int index_begin = 1);   // прочитать все аргументы
int push(lua_State* L) const;                   // положить все, вернуть число
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
    lua_State *vm;        // VM вызывающего ресурса
    std::string resource; // имя ресурса
};
```

## mta::lua::rest_args

```cpp
struct rest_args
{
    Arguments values;     // все хвостовые аргументы
};
```

## Ошибки

```cpp
[[noreturn]] void raise(std::string message);        // бросить → Lua-ошибка
template<typename... A> [[noreturn]] void raise_error(A&&... args);  // стримится через <<
```

Любое C++-исключение в функции модуля превращается в Lua-ошибку трамплином
`mta::lua::protected_call`. Серверный процесс защищён от исключений.

## mta::async::Callback

Стабильная ссылка на Lua-функцию, переживающая рестарты ресурсов. Move-only.

```cpp
static Callback from_stack(lua_State* L, int index); // привязать функцию (бросает на не-функции)
bool valid() const;
const std::string& resource() const;                 // имя ресурса-владельца
bool call(const mta::lua::Arguments&) const;         // вызвать; false, если ресурс мёртв/ошибка
```

## mta::async::Scheduler

Фоновые задачи с доставкой результатов в главный поток (DoPulse).

```cpp
static Scheduler& instance();
void start();      // поднять воркеров (зовится при инициализации)
void stop();       // остановить воркеров и сбросить очереди (shutdown)
void pump();       // главный поток: раздать результаты, сработать таймеры

void post_task(std::function<Arguments()> work,
               std::function<void(const Arguments&, const char*)> completion);
// work — на воркере (БЕЗ Lua!), completion — на главном потоке;
// error == nullptr при успехе.

std::uint64_t post_timer(std::string resource, int delay_ms, int repeat_count,
                         std::function<void(std::uint64_t)> completion);
// completion(tick) каждые delay_ms; repeat_count раз (0 = бесконечно).

bool cancel_timer(std::uint64_t id);
void handle_resource_stopped(const std::string& resource);
bool running() const;
```

## mta::resources::Store<T>

Пер-ресурсные данные с автоочисткой при остановке ресурса.

```cpp
template<typename T> class Store
{
    T& for_state(lua_State* L);               // данные вызывающего ресурса
    T* try_find(const std::string& resource); // или nullptr
    // on_resource_stopped / on_all_released — очистка автоматическая
};
```

## mta::log

Уровни (по возрастанию): `Debug < Info < Warn < Error < Off`. Сообщение
печатается, если его уровень >= текущего. По умолчанию `Info`.

```cpp
enum class Level { Debug, Info, Warn, Error, Off };
void set_level(Level);
Level get_level();

template<typename... A> void debug(lua_State*, A&&...);  // привязано к ресурсу
template<typename... A> void info(A&&...);    // консоль сервера
template<typename... A> void warn(A&&...);    // консоль сервера (предупреждение)
template<typename... A> void error(A&&...);   // консоль сервера (ошибка)
```

## mta::events

Триггер MTA-событий: модуль «бросает» событие в Lua-скрипты ресурса через
штатный `triggerEvent` (источник — `root`).

```cpp
bool trigger(lua_State* L, const char* event_name, const mta::lua::Arguments& args);
// false, если triggerEvent недоступен или вызов не удался.
```

## mta::lua — хелперы таблиц

```cpp
// Конвертация Argument → C++-тип (бросает при несовпадении).
template<typename T> T convert(const Argument&);

// Чтение поля по строковому ключу.
template<typename T> T get_field(const Table&, const char* key, T default_value);
template<typename T> T get_field(const Table&, const char* key);  // бросает, если нет

// Запись (или перезапись) поля.
void set_field(Table&, const char* key, Argument value);
```

## mta::userdata::Registry<T>

Объекты с методами и `__gc` (деструктором).

```cpp
template<typename T> class Registry
{
    using Registrar = void (*)(lua_State*);
    static void set_methods(Registrar);   // один раз на процесс
    static void ensure(lua_State*);       // метатаблица + методы в этом VM
    static T* create(lua_State*, T value); // userdata на стеке, возвращает T*
    static T* check(lua_State*, int index); // проверка userdata (бросает)
    template<std::size_t Tag, typename F>
    static void add_method(lua_State*, const char* name, F fn);
};

// Регистрация метода: MTA_METHOD(Type, "имя", лямбда);
// Лямбда принимает self (Type&) первым параметром.
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
