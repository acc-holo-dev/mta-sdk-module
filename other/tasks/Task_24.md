# Task 24 — Source code quality

**Источник:** PROMT.md — разделы: 46 — Source code quality
**Область задачи:** Требования к качеству исходного кода SDK — C++20, RAII, явный ownership, документированные thread safety и lifetime, отсутствие hidden Lua cross-thread access, undefined behavior и compiler-specific трюков, включённые warnings.

---

# 46. Source code quality

Требования:

* C++20;
* RAII;
* no unnecessary global mutable state;
* explicit ownership;
* no raw ownership;
* thread safety documented;
* lifetime documented;
* no hidden Lua cross-thread access;
* no undefined behavior;
* no compiler-specific tricks for public identity;
* warnings enabled;
* warnings treated as errors where practical.

---

## Чек-лист соответствия проекта

- [ ] Код SDK собирается как C++20 (в CMake установлен стандарт C++20).
- [ ] Соблюдается RAII: ресурсы освобождаются автоматически (деструкторы, guard-типы).
- [ ] Отсутствует unnecessary global mutable state.
- [ ] Ownership явный (explicit ownership); raw ownership не используется.
- [ ] Thread safety и lifetime задокументированы (в коде или документации).
- [ ] Отсутствует hidden Lua cross-thread access (прямой доступ к Lua state из других потоков).
- [ ] Отсутствует undefined behavior.
- [ ] Не используются compiler-specific tricks для public identity.
- [ ] Warnings включены и трактуются как ошибки там, где это практично.