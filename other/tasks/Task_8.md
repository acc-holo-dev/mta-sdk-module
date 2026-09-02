# Task 8 — Документация аргументов, signature metadata и mta docs

**Источник:** PROMT.md — разделы: 8 — Обязательная документация аргументов; 9 — Signature metadata; 10 — `mta docs`
**Область задачи:** Задача покрывает обязательную документацию аргументов для developer, metadata сигнатур зарегистрированных функций и генерацию документации командой `mta docs`.

---

# 8. Обязательная документация аргументов

`other/documents/example.md` должен подробно объяснить developer:

* как объявляется function;
* как понять какие аргументы принимает function;
* какие типы поддерживаются;
* как работают optional arguments;
* как работают variadic arguments;
* как возвращаются результаты;
* как генерируется error;
* как писать manual validation;
* когда automatic binding недостаточен.

Обязательно дать examples:

```cpp
MTA_FUNCTION("sum",
    [](double a, double b)
    {
        return a + b;
    });
```

и Lua:

```lua
sum(10, 20)
```

затем:

```lua
sum(10)
```

результат:

```text
argument #2 is missing
```

затем:

```lua
sum("10", 20)
```

результат:

```text
argument #1 has invalid type
```

# 9. Signature metadata

Каждая зарегистрированная function должна по возможности иметь metadata:

```text
name
description
arguments
return values
flags
category
```

Например:

```text
name:
    sum

description:
    Adds two numbers.

arguments:
    #1 number
    #2 number

returns:
    #1 number
```

Эта metadata должна использоваться для:

* diagnostics;
* `mta docs`;
* API inspection;
* future tooling.

Не заставлять developer описывать metadata дважды, если её можно вывести из C++ signature.

# 10. `mta docs`

Реализовать:

```bash
mta docs
```

Команда должна анализировать зарегистрированные functions/object APIs и генерировать developer documentation.

Результат должен быть читаемым markdown.

Например:

```text
generated/
    api.md
```

или другое логичное место.

Документация должна включать:

* function name;
* description;
* argument types;
* optional arguments;
* return types;
* object methods;
* errors/flags, если они доступны.

Если какую-либо информацию невозможно вывести автоматически — это должно быть явно отражено в архитектуре и документации.

---

## Чек-лист соответствия проекта

- [ ] Существует файл `other/documents/example.md`, подробно объясняющий объявление function, принимаемые аргументы и их типы, optional и variadic arguments, возврат результатов, генерацию error, manual validation и случаи, когда automatic binding недостаточен.
- [ ] В документации присутствует пример C++ с `MTA_FUNCTION("sum", ...)` и Lua-примеры вызовов `sum(10, 20)`, `sum(10)`, `sum("10", 20)`.
- [ ] В документации приведены результаты ошибочных вызовов: `argument #2 is missing` и `argument #1 has invalid type`.
- [ ] Каждая зарегистрированная function по возможности имеет metadata: name, description, arguments, return values, flags, category.
- [ ] Metadata сигнатур используется для diagnostics, `mta docs`, API inspection и future tooling.
- [ ] Metadata автоматически выводится из C++ signature — developer не описывает её дважды.
- [ ] Реализована команда `mta docs`, анализирующая зарегистрированные functions/object APIs и генерирующая developer documentation в виде читаемого markdown (например, `generated/api.md` или другое логичное место).
- [ ] Сгенерированная документация включает function name, description, argument types, optional arguments, return types, object methods и errors/flags (если доступны).
- [ ] Если какую-либо информацию невозможно вывести автоматически, это явно отражено в архитектуре и документации.