# Продвинутые гайды — ml_base

Подробные руководства по неочевидным темам. Быстрый старт и базовые рецепты
см. в [README.md](../README.md).

## Содержание

1. [Потокобезопасность](#1-потокобезопасность)
2. [Асинхронность и таймеры](#2-асинхронность-и-таймеры)
3. [Таблицы: глубокое чтение](#3-таблицы-глубокое-чтение)
4. [Пер-ресурсное состояние](#4-пер-ресурсное-состояние)
5. [Ошибки и исключения](#5-ошибки-и-исключения)
6. [Прямой доступ к стеку](#6-прямой-доступ-к-стеку)

---

## 1. Потокобезопасность

**Главное правило: Lua трогаем только в главном потоке сервера.**

- Каждый ресурс MTA живёт в своём `lua_State`. VM **не потокобезопасен**.
- Вызовы Lua из воркеров — верный способ уронить сервер.
- Поэтому фоновые задачи (`post_task`) выполняют **чистый C++** без Lua,
  а результат доставляется в главный поток через `DoPulse` (`pump()`).

Потоковая модель модуля:

```
главный поток (сервер)          воркеры (Scheduler)
─────────────────────          ─────────────────────
  вызов Lua-функции               work() — чистый C++
  post_task(...)  ───────────────► выполняет работу
  ...                             кладёт результат
  DoPulse → pump() ◄────────────── (мьютекс)
  completion() — снова Lua ✓
```

Из этого следуют правила:

1. Никогда не храните `lua_State*` между вызовами (VM умирает при остановке ресурса).
2. Для отложенного вызова Lua используйте `mta::async::Callback`.
3. Для данных на ресурс — `mta::resources::Store`.

## 2. Асинхронность и таймеры

### Фоновая задача

```cpp
MTA_LUA_FUNCTION("my_fetch", "Считает в фоне и зовёт callback.")
{
    auto [url, callback] = mta::lua::args<std::string, mta::async::Callback>(L);
    auto cb = std::make_shared<mta::async::Callback>(std::move(callback));

    mta::async::Scheduler::instance().post_task(
        [url]() {                        // ВОРКЕР: без Lua!
            mta::lua::Arguments result;
            result.push_string(do_http_get(url));
            return result;
        },
        [cb](const mta::lua::Arguments &result, const char *error) {
            if (error != nullptr) {      // ГЛАВНЫЙ поток (DoPulse)
                mta::log::error("my_fetch: ", error);
                return;
            }
            cb->call(result);            // можно звать Lua
        });

    return mta::lua::push_results(L, true);
}
```

### Важные детали

- `Callback` move-only: при захвате в `std::function` оборачивайте в
  `std::make_shared` (std::function требует копируемых целей).
- Если `work()` бросает исключение, оно ловится воркером и приходит в
  `completion` как строка `error`.
- `Callback::call` вернёт `false`, если ресурс-владелец уже остановлен —
  старый callback после рестарта ресурса не выстрелит.

### Таймер

```cpp
MTA_LUA_FUNCTION("my_every", "Зовёт callback каждые N мс.")
{
    auto [delay, callback] = mta::lua::args<std::int64_t, mta::async::Callback>(L);
    auto cb = std::make_shared<mta::async::Callback>(std::move(callback));

    const auto id = mta::async::Scheduler::instance().post_timer(
        cb->resource(),                       // таймер живёт, пока жив ресурс
        static_cast<int>(delay), 0,           // 0 = бесконечно
        [cb](std::uint64_t tick) {
            mta::lua::Arguments args;
            args.push_number(static_cast<lua_Number>(tick));
            cb->call(args);
        });

    return mta::lua::push_results(L, static_cast<lua_Number>(id));
}
```

Таймер автоматически отменяется при остановке ресурса-владельца (имя ресурса
берётся из `cb->resource()`).

## 3. Таблицы: глубокое чтение

`mta::lua::Argument` читает таблицу рекурсивно, до 32 уровней (`max_table_depth`).
Вложенные таблицы — те же `Argument` внутри `Table`.

```cpp
// Lua: {10, 20, {30, 40}, name = "x"}
MTA_LUA_FUNCTION("my_flatten", "Сумма всех чисел в таблице.")
{
    auto [table] = mta::lua::args<mta::lua::Table>(L);

    double sum = 0.0;
    std::function<void(const mta::lua::Table&)> walk =
        [&](const mta::lua::Table &t) {
            for (const auto &v : t.array) {
                if (v.type() == mta::lua::Argument::Type::Number) sum += v.as_number();
                else if (v.is_table()) walk(v.as_table());
            }
            for (const auto &[k, v] : t.fields) {
                (void)k;
                if (v.type() == mta::lua::Argument::Type::Number) sum += v.as_number();
                else if (v.is_table()) walk(v.as_table());
            }
        };
    walk(table);

    return mta::lua::push_results(L, sum);
}
```

Полезные детали:

- `Table.array` — только целочисленные ключи 1..n; дыры заполняются nil.
- `Table.fields` — только строковые ключи; другие ключи отбрасываются.
- Циклическая таблица не зациклит модуль: глубже 32 уровней — обрыв.
- Для результата-таблицы: соберите `mta::lua::Table` и верните через
  `push_results(L, mta::lua::Argument(std::move(table)))`.

## 4. Пер-ресурсное состояние

Каждый ресурс — свой VM, и он умирает при остановке. Хранить данные на ресурс
правильно так:

```cpp
namespace
{
struct Session { std::string token; int requests = 0; };
mta::resources::Store<Session> g_sessions;   // один статик в .cpp
}

MTA_LUA_FUNCTION("session_hit", "Счётчик обращений ресурса.")
{
    Session &s = g_sessions.for_state(L);    // создать/получить
    ++s.requests;
    return mta::lua::push_results(L, static_cast<lua_Number>(s.requests));
}
```

Очистка полностью автоматическая: при `ResourceStopped` запись ресурса
стирается, при `ShutdownModule` — всё целиком. Ручных cleanup-функций нет.

## 5. Ошибки и исключения

- **Внутри функции** бросайте `mta::lua::raise_error("...")` — стек C++
  раскручивается корректно, Lua-скриптер получит понятную ошибку.
- **Проверку типов** делает `args<...>` сам — писать руками не нужно.
- **Любое** непойманное C++-исключение перехватывается трамплином и
  становится Lua-ошибкой: сервер не падает.
- Не зовите `luaL_error`/`luaL_check*` напрямую — это longjmp поверх
  C++-объектов (утечки деструкторов). Используйте `raise_error`.

Типичные сообщения (генерируются автоматически):

```
argument #1 must be a number, got string
argument #2 must be a string, got table
argument #3 must be an integer, got no value   ← аргумент не передан
```

## 6. Прямой доступ к стеку

В теле `MTA_LUA_FUNCTION` доступен `lua_State *L` — можно звать любые функции
Lua 5.1 C API напрямую (когда `args<...>` не подходит):

```cpp
MTA_LUA_FUNCTION("my_dump", "Типы всех аргументов.")
{
    const int count = lua_gettop(L);
    lua_pushnumber(L, static_cast<lua_Number>(count));
    for (int i = 1; i <= count; ++i)
    {
        lua_pushstring(L, lua_typename(L, lua_type(L, i)));
    }
    return count + 1;
}
```

Исключения и тут ловятся каркасом. Не храните `L` между вызовами.
