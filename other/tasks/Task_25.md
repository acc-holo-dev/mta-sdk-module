# Task 25 — Definition of Done

**Источник:** PROMT.md — разделы: 50 — Definition of Done
**Область задачи:** Definition of Done для V2 — полный перечень условий (архитектура, developer workflow, function creation, arguments, lifecycle, async, userdata, документация, doctor, server, release), при одновременном выполнении которых V2 считается завершённой.

---

# 50. Definition of Done

V2 считается завершённой только если одновременно выполняются все условия.

## Architecture

```text
source/functions
source/library
source/sdk
other/tests
other/server
other/tools
other/documents
other/third_party
```

разделены.

---

## Developer workflow

Новый developer может:

```bash
mta init test-module
mta new function hello
mta build
mta test
```

без ручного редактирования CMake registry files.

---

## Function creation

Developer пишет одну `.cpp`.

Не обязан:

* добавлять function в registry;
* менять CMake;
* регистрировать function ещё где-то.

---

## Arguments

Binder автоматически обнаруживает:

* count;
* type;
* optional;
* variadic;
* callback.

Ошибки понятные.

---

## Lifecycle

После resource restart:

```text
old callback = invalid
old task completion = invalid
old timer = invalid
old state = cleaned
```

И не может попасть в новую resource generation.

---

## Async

Worker threads никогда не вызывают Lua напрямую.

---

## Userdata

Stable identity.

No `typeid(T).name()` as public identity.

---

## Documentation

`example.md` позволяет новому developer самостоятельно создать:

* function;
* async function;
* timer;
* callback;
* object;
* state;
* error handling.

---

## Doctor

```bash
mta doctor
```

реально проверяет environment.

---

## Server

```bash
mta test integration
```

может поднять controlled MTA server и проверить реальный module load/function execution.

---

## Release

GitHub Release публикует только:

```text
<module>.dll
<module>.so
```

после успешного build/test pipeline.

---

## Чек-лист соответствия проекта

- [ ] Каталоги `source/functions`, `source/library`, `source/sdk`, `other/tests`, `other/server`, `other/tools`, `other/documents`, `other/third_party` существуют и разделены.
- [ ] Новый developer может выполнить `mta init test-module`, `mta new function hello`, `mta build`, `mta test` без ручного редактирования CMake registry files.
- [ ] Для создания новой function developer пишет одну `.cpp` и не обязан добавлять function в registry, менять CMake или регистрировать function ещё где-то.
- [ ] Binder автоматически обнаруживает count, type, optional, variadic и callback аргументов; ошибки понятные.
- [ ] После resource restart старые callback/task completion/timer/state становятся invalid/cleaned и не попадают в новую resource generation.
- [ ] Worker threads никогда не вызывают Lua напрямую.
- [ ] Userdata имеет stable identity; `typeid(T).name()` не используется как public identity.
- [ ] `example.md` позволяет новому developer самостоятельно создать function, async function, timer, callback, object, state и error handling.
- [ ] `mta doctor` реально проверяет environment; `mta test integration` может поднять controlled MTA server и проверить реальный module load/function execution.
- [ ] GitHub Release публикует только `<module>.dll` и `<module>.so` после успешного build/test pipeline.