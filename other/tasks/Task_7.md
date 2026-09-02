# Task 7 — Binder V2: биндинг, валидация аргументов и ошибки

**Источник:** PROMT.md — разделы: 6 — Binder V2; 7 — Argument validation; 19 — Errors; фазы: PHASE 4 — Binder V2
**Область задачи:** Описывает Binder V2 (typed binding с автоматической конвертацией аргументов и результатов), автоматическую валидацию аргументов с понятными Lua errors и единую error model (PHASE 4).

---

# 6. Binder V2

Binder является одной из главных частей SDK.

Не уничтожать существующую typed-binding идею.

Нужно сохранить возможность писать:

```cpp
MTA_FUNCTION("sum", [](double a, double b)
{
    return a + b;
});
```

или аналогичный простой API.

Цель:

```text
Lua arguments
    ↓
automatic type conversion
    ↓
C++ function
    ↓
automatic result conversion
```

Поддержать минимум:

* integer;
* number;
* boolean;
* string;
* table;
* optional;
* variadic/rest arguments;
* callback;
* resource context;
* native MTA objects;
* multiple return values;
* errors.

# 7. Argument validation

Developer НЕ должен вручную проверять каждый простой argument.

Например:

```cpp
MTA_FUNCTION("sum",
    [](double a, double b)
    {
        return a + b;
    });
```

SDK должен автоматически:

1. проверить наличие аргументов;
2. проверить количество;
3. проверить тип;
4. конвертировать значение;
5. вызвать C++ function;
6. вернуть Lua result.

Если Lua вызвал:

```lua
sum(10)
```

framework должен выдать понятную Lua error.

Например:

```text
bad argument #2 to 'sum'
expected number, got nil
```

Если:

```lua
sum("hello", 10)
```

то:

```text
bad argument #1 to 'sum'
expected number, got string
```

Если требуется количество:

```lua
bad argument count to 'sum'
expected 2 arguments, got 1
```

Сообщения должны быть:

* короткими;
* понятными;
* диагностичными;
* пригодными для production logs.

# 19. Errors

Создать единый error model.

Минимум:

```text
InvalidArgument
InvalidType
MissingArgument
ResourceStopped
InvalidCallback
InvalidObject
AsyncCancelled
InternalError
```

Binder должен преобразовывать ошибки в понятные Lua errors.

Пример:

```text
bad argument #1 to 'foo':
expected number, got string
```

А internal error должен быть различим от user input error.

---

## Фазы реализации (из раздела 49 PROMT.md)

## PHASE 4 — Binder V2

Рефакторинг:

```text
argument conversion
result conversion
traits
invoke
errors
signature metadata
```

Не менять behavior без regression tests.

---

## Чек-лист соответствия проекта

- [ ] Сохранена возможность писать `MTA_FUNCTION("sum", [](double a, double b) { return a + b; });` — typed-binding идея не уничтожена.
- [ ] Binder поддерживает минимум: integer, number, boolean, string, table, optional, variadic/rest arguments, callback, resource context, native MTA objects, multiple return values, errors.
- [ ] SDK автоматически проверяет наличие аргументов, количество, тип и конвертирует значение перед вызовом C++ function.
- [ ] В developer code (`source/functions/`) отсутствуют ручные проверки каждого простого argument.
- [ ] Реализованы сообщения об ошибках формата `bad argument #N to 'func'` / `expected number, got nil` / `bad argument count to 'sum'` / `expected 2 arguments, got 1`.
- [ ] Сообщения об ошибках короткие, понятные, диагностичные и пригодные для production logs.
- [ ] Существует единая error model, включающая InvalidArgument, InvalidType, MissingArgument, ResourceStopped, InvalidCallback, InvalidObject, AsyncCancelled, InternalError.
- [ ] Binder преобразовывает ошибки в понятные Lua errors; internal error различим от user input error.
- [ ] Выполнен рефакторинг argument conversion, result conversion, traits, invoke, errors и signature metadata (PHASE 4).
- [ ] Существуют regression tests, обеспечивающие неизменность behavior при рефакторинге binder'а.