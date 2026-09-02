# Task 19 — Third-party зависимости и build system

**Источник:** PROMT.md — разделы: 34 — Third-party dependencies; 35 — Build system
**Область задачи:** Задача покрывает reproducible vendor approach для third-party зависимостей (mta-sdk, Lua) с проверкой соответствия Lua headers и compiled Lua source, а также требования к build system на базе CMake с user-facing командой `mta build`.

---

# 34. Third-party dependencies

Сохранить reproducible vendor approach:

```text
other/third_party/mta-sdk
other/third_party/lua
```

Не использовать системную Lua installation для module ABI, если это нарушает MTA ABI requirements.

Проверять соответствие:

```text
Lua headers
+
compiled Lua source
```

Так, чтобы случайная несовместимая Lua implementation не могла попасть в build.

# 35. Build system

Сохранить:

* CMake;
* CMake Presets;
* automatic source discovery;
* MSVC;
* MinGW;
* Linux;
* tests;
* LTO;
* unity build при необходимости.

Но сделать user-facing workflow:

```bash
mta build
```

Developer не должен знать детали CMake command line.

Advanced users всё ещё должны иметь возможность использовать CMake напрямую.

---

## Чек-лист соответствия проекта

- [ ] Сохранён reproducible vendor approach: зависимости находятся в `other/third_party/mta-sdk` и `other/third_party/lua`.
- [ ] Системная Lua installation не используется для module ABI, если это нарушает MTA ABI requirements.
- [ ] Проверяется соответствие Lua headers + compiled Lua source — случайная несовместимая Lua implementation не может попасть в build.
- [ ] Build system сохраняет: CMake, CMake Presets, automatic source discovery, поддержку MSVC, MinGW и Linux, tests, LTO, unity build при необходимости.
- [ ] User-facing workflow сборки — команда `mta build`; developer не должен знать детали CMake command line.
- [ ] Advanced users могут использовать CMake напрямую (CMakeLists.txt и CMakePresets.json присутствуют и работоспособны).