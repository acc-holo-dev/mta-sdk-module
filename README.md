# MTA:SA Lua Module — ml_base

Крепкая основа для серверного модуля [MTA:SA](https://multitheftauto.com):
динамическая библиотека (`ml_base.dll` / `ml_base.so`), которую загружает
MTA-сервер и которая добавляет в Lua собственные нативные функции.

Функции пишутся **обычным C++ с телом**, аргументы читаются по типам сами —
никаких ручных `check_number`, индексов и «это число, это строка»:

```cpp
// src/functions/basics/my_sum.cpp
#include "registry/registry.hpp"

MTA_LUA_FUNCTION("my_sum", "Складывает два числа.")
{
    auto [a, b] = mta::lua::args<double, double>(L);   // типы сами, проверки сами
    return mta::lua::push_results(L, a + b);           // возврат тоже авто
}
```

Пересобрал — `my_sum` уже видна во всех ресурсах сервера. Новые .cpp
подхватываются автоматически, регистрация — тоже.

---

## Содержание

- [Архитектура](#архитектура)
- [Сборка](#сборка)
- [Установка на сервер](#установка-на-сервер)
- [Написание функций](#написание-функций)
- [Типы аргументов и результатов](#типы-аргументов-и-результатов)
- [Правила безопасности](#правила-безопасности)
- [Тесты](#тесты)

## Архитектура

```
src/
├── module/       # контракт MTA: mta::module — init, pulse, resource hooks
├── lua/          # работа со стеком Lua: mta::lua
│   ├── bind.hpp        👉 биндер: args<...>, pull/push, границы исключений
│   ├── argument.hpp    #   Argument: значение Lua + таблицы
│   ├── arguments.hpp   #   Arguments: список значений
│   └── protect.hpp     #   исключения → Lua-ошибки
├── registry/     # реестр + макросы MTA_LUA_FUNCTION / MTA_LUA_FUNC
├── runtime/      # движок: scheduler, callback, resources, logging
└── functions/    👉 ЕДИНСТВЕННАЯ папка, которую трогаешь ты
    ├── basics/   # sample_add, sample_echo, sample_greet, sample_tag,
    │            #   sample_minmax, sample_range
    ├── tables/   # sample_table_stats
    ├── info/     # sample_version, module_functions
    ├── async/    # sample_async_add, sample_timer(+cancel)
    └── raw/      # sample_stack_dump — прямой доступ к стеку

tests/            # embedded-Lua харнесс + скрипты
vendor/           # mta-sdk (заголовки SDK) + lua (Lua 5.1.5)
cmake/            # инфраструктура сборки
```

Свои функции раскладывай по доменам внутри `functions/` — например
`functions/crypto/`, `functions/http/`. Домен создаётся простым добавлением
папки с .cpp: всё дерево `src/**/*.cpp` собирается автоматически.

## Сборка

Требуется CMake ≥ 3.27, Ninja и компилятор с C++20 и std::thread
(MinGW-w64 posix-threads, MSVC 2019+ или GCC/Clang на Linux).

```bash
# Windows (MinGW-w64)
cmake --preset win-mingw && cmake --build --preset win-mingw

# Windows (MSVC)
cmake --preset win-msvc && cmake --build --preset win-msvc

# Linux (GCC)
cmake --preset linux-gcc && cmake --build --preset linux-gcc
```

Артефакт: `build/<пресет>/module/<платформа>-<арх>/ml_base.dll`
(например `module/win-x64/ml_base.dll`), рантайм MinGW линкуется статически.

## Установка на сервер

1. Скопируйте `ml_base.dll` в `mods/deathmatch/modules/` сервера.
2. В `mtaserver.conf` добавьте `<module src="ml_base"/>`.
3. Перезапустите сервер: в консоли появится
   `MODULE: Loaded "Base Module" (1.00) by "anon"`.

Битность модуля = битность сервера (современный MTA — x64).

## Написание функций

### Простая функция

```cpp
#include "registry/registry.hpp"

MTA_LUA_FUNCTION("my_sum", "Складывает два числа.")
{
    auto [a, b] = mta::lua::args<double, double>(L);
    return mta::lua::push_results(L, a + b);
}
```

`mta::lua::args<...>(L)` читает аргументы по порядку: тип каждого проверяется
автоматически, при неверном типе Lua-скриптер получит
`argument #N must be <тип>, got <факт>`. Лишние аргументы игнорируются,
недостающие дают `…got no value`.

### Типы и несколько результатов

```cpp
MTA_LUA_FUNCTION("my_div", "Делит два числа.")
{
    auto [a, b] = mta::lua::args<double, double>(L);
    if (b == 0.0)
    {
        mta::lua::raise_error("my_div: деление на ноль");  // → ошибка Lua
    }
    return mta::lua::push_results(L, a / b);
}

// несколько результатов за вызов:
MTA_LUA_FUNCTION("my_minmax", "Минимум и максимум.")
{
    auto [a, b] = mta::lua::args<double, double>(L);
    return mta::lua::push_results(L, std::min(a, b), std::max(a, b));
}

// переменное число результатов — через Arguments:
MTA_LUA_FUNCTION("my_range", "Числа от from до to.")
{
    auto [from, to] = mta::lua::args<std::int64_t, std::int64_t>(L);
    mta::lua::Arguments result;
    for (auto i = from; i <= to; ++i) result.push_number(static_cast<lua_Number>(i));
    return result.push(L);
}
```

### Необязательные аргументы

```cpp
// std::optional<T>: nil или отсутствие → nullopt, дефолт через value_or.
MTA_LUA_FUNCTION("my_greet", "Приветствие.")
{
    auto [name, greeting] = mta::lua::args<std::string, std::optional<std::string>>(L);
    return mta::lua::push_results(L, greeting.value_or("привет") + ", " + name);
}
```

### Таблицы

```cpp
MTA_LUA_FUNCTION("my_table_demo", "Пример работы с таблицей.")
{
    auto [table] = mta::lua::args<mta::lua::Table>(L);   // не-таблица → ошибка
    double sum = 0.0;
    for (const auto &value : table.array)
    {
        if (value.type() == mta::lua::Argument::Type::Number)
        {
            sum += value.as_number();
        }
    }

    mta::lua::Table result;
    result.fields.emplace_back("sum", mta::lua::Argument(sum));
    return mta::lua::push_results(L, mta::lua::Argument(std::move(result)));
}
```

`mta::lua::Argument` принимает ЛЮБОЕ значение (таблицы рекурсивно, до 32
уровней).

### Вариадика (неизвестное число аргументов)

```cpp
MTA_LUA_FUNCTION("my_echo", "Возвращает все аргументы обратно.")
{
    mta::lua::Arguments arguments;
    arguments.read(L);
    lua_settop(L, 0);
    return arguments.push(L);
}
```

### Асинхронная функция и таймеры

```cpp
#include <memory>
#include "lua/arguments.hpp"
#include "registry/registry.hpp"
#include "runtime/callback.hpp"
#include "runtime/logging.hpp"
#include "runtime/scheduler.hpp"

MTA_LUA_FUNCTION("my_async", "Считает в фоне; callback(result) на DoPulse.")
{
    auto [value, callback] = mta::lua::args<double, mta::async::Callback>(L);
    auto cb = std::make_shared<mta::async::Callback>(std::move(callback));

    mta::async::Scheduler::instance().post_task(
        [value] {                       // фоновый поток: БЕЗ Lua!
            mta::lua::Arguments result;
            result.push_number(heavy(value));
            return result;
        },
        [cb](const mta::lua::Arguments &result, const char *error) {
            if (error != nullptr) { mta::log::error("my_async: ", error); return; }
            cb->call(result);           // главный поток: безопасно
        });

    return mta::lua::push_results(L, true);
}
```

`Callback` — тип параметра: привязка Lua-функции, переживание рестартов
ресурса и авто-освобождение делаются каркасом. Таймеры — см.
`src/functions/async/timers.cpp` (`sample_timer`/`sample_timer_cancel`).
Callback move-only — при захвате в completion оборачивай в `make_shared`.

### Пер-ресурсное состояние

```cpp
namespace
{
struct MySession { std::string token; int requests = 0; };
mta::resources::Store<MySession> g_sessions;   // статик в .cpp
}

MTA_LUA_FUNCTION("my_session_hit", "Счётчик обращений ресурса.")
{
    MySession &session = g_sessions.for_state(L);   // VM доступен прямо в теле
    ++session.requests;
    return mta::lua::push_results(L, static_cast<lua_Number>(session.requests));
}
```

Запись стирается автоматически при остановке ресурса.

### Логирование

```cpp
mta::log::info("кэш загружен: ", count, " записей");
mta::log::error("загрузка не удалась: ", code);
mta::log::debug(L, "вызвано с ", n, " аргументами");   // привязано к ресурсу
```

### Прямой доступ к стеку (экзотика)

В теле функции доступен `lua_State *L` — можно делать что угодно напрямую:

```cpp
MTA_LUA_FUNCTION("my_raw", "Описание.")
{
    return lua_gettop(L);   // любой низкоуровневый код; исключения ловит каркас
}
```

Живой пример — `src/functions/raw/stack_dump.cpp`.

### Лямбда-стиль (короткие однострочники)

Для однострочных функций есть второй макрос — `MTA_LUA_FUNC` с лямбдой:
типы читаются из сигнатуры, возврат автоматический:

```cpp
MTA_LUA_FUNC("my_sum", "Складывает два числа.",
    [](double a, double b) { return a + b; });
```

## Типы аргументов и результатов

| Аргумент в `args<...>` | Из Lua |
|---|---|
| `double`, `float` | число |
| `int`, `int64_t`, … | целое (с проверкой диапазона) |
| `bool` | boolean |
| `std::string` / `std::string_view` | строка |
| `mta::lua::Argument` | любое значение (таблицы рекурсивно) |
| `mta::lua::Table` | таблица |
| `mta::async::Callback` | функция (стабильная ссылка) |
| `std::optional<T>` | T или nil/ничего |

| Результат в `push_results` | В Lua |
|---|---|
| значение (число/строка/bool/таблица/…) | один результат |
| несколько значений через запятую | несколько результатов |
| `mta::lua::Arguments` (+ `push`) | целый список результатов |
| `nullptr` | nil |

Неправильный тип аргумента превращается в понятную Lua-ошибку вида
`argument #1 must be a number, got string` — прямо из `args<...>`, писать
проверки руками не нужно.

## Правила безопасности

1. **Никогда не храните `lua_State *` между вызовами.** VM ресурса умирает при
   его остановке. Для отложенных вызовов — `mta::async::Callback`; для данных —
   `mta::resources::Store`.
2. **Lua трогаем только в главном потоке.** Из воркеров — чистый C++,
   результаты через `post_task` → `DoPulse`.
3. **Функции глобальны для всех ресурсов** — давайте уникальные имена.
4. **Исключения не покидают модуль**: макросы переводят всё в Lua-ошибки;
   стек C++ раскручивается корректно.
5. **Битность модуля = битность сервера.**

## Тесты

```bash
cmake --build --preset win-mingw --target sdk_tests
ctest --preset win-mingw            # или запустить sdk_tests.exe напрямую
```

Харнесс (`tests/harness.cpp`) поднимает чистый Lua 5.1, ставит mock-менеджер
и гоняет `tests/scripts/*.lua`: базовые функции, таблицы, асинхронность,
таймеры и все фичи биндера (optional, несколько результатов, вариадика,
прямой стек). Добавляй свои скрипты — подхватываются автоматически.

---

Имя модуля и автор — в `src/module/module.cpp` (`module_details`),
имя DLL — в `CMakeLists.txt` (`OUTPUT_NAME`).
