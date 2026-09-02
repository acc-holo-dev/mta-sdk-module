# Task 2 — Правило именования и module identity

**Источник:** PROMT.md — разделы: 2 — Критическое правило именования; 5 — Module identity
**Область задачи:** Закрепляет критическое правило именования (SDK не добавляет автоматически свои namespace/prefix к именам функций и модулей) и единый источник module identity, задаваемый developer.

---

# 2. Критическое правило именования

SDK НЕ ДОЛЖЕН автоматически добавлять свои namespace/prefix к именам функций или модулей.

Неправильно:

```cpp
MTA_FUNCTION("holo.crypto.sha256")
```

если developer хочет назвать функцию:

```cpp
"sha256"
```

SDK должен зарегистрировать именно:

```text
sha256
```

То же самое относится к названию native module.

Имя module полностью определяется developer через `config/module.toml`.

SDK не должен автоматически добавлять:

```text
mta.
sdk.
holo.
module.
```

и другие namespace.

Namespacing может быть указан developer вручную:

```cpp
MTA_FUNCTION("crypto.sha256", ...)
```

но framework не должен изменять это имя.

# 5. Module identity

Module name/title/author/version должны задаваться developer.

Не генерировать:

```text
holo_
mta_
sdk_
```

и другие prefixes.

Module identity должна использоваться:

* в CMake;
* в resulting DLL/SO;
* в metadata;
* в diagnostics;
* в packaging;
* в MTA module registration.

Не должно быть нескольких независимых источников истины.

---

## Чек-лист соответствия проекта

- [ ] Имя функции, указанное developer в `MTA_FUNCTION("sha256", ...)`, регистрируется в Lua без изменений — без добавленных prefixes.
- [ ] Отсутствует код, автоматически добавляющий prefixes «mta.», «sdk.», «holo.», «module.» и другие namespace к именам функций или модулей.
- [ ] Namespacing, указанный developer вручную (например, `MTA_FUNCTION("crypto.sha256", ...)`), не изменяется framework'ом.
- [ ] Имя native module полностью определяется developer через `config/module.toml`.
- [ ] Отсутствует код, генерирующий prefixes «holo_», «mta_», «sdk_» и другие для module identity.
- [ ] Module identity (name/title/author/version) задаётся developer, а не генерируется SDK.
- [ ] Module identity используется в CMake, resulting DLL/SO, metadata, diagnostics, packaging и MTA module registration.
- [ ] Отсутствует несколько независимых источников истины для module identity.