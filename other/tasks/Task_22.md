# Task 22 — CI

**Источник:** PROMT.md — фазы: PHASE 13 — CI
**Область задачи:** Настройка CI — обязательные шаги (build, unit, lua, integration, doctor) и минимальная матрица окружений (Windows MSVC, Windows MinGW, Linux GCC, при наличии разумной инфраструктуры — Clang).

---

## Требования

## PHASE 13 — CI

CI должен выполнять:

```text
build
unit
lua
integration
doctor
```

где environment позволяет.

Matrix минимум:

```text
Windows MSVC
Windows MinGW
Linux GCC
```

При наличии разумной инфраструктуры добавить Clang.

---

## Чек-лист соответствия проекта

- [ ] В репозитории существует настроенная CI-конфигурация (CI workflow).
- [ ] CI выполняет build проекта.
- [ ] CI выполняет unit-тесты.
- [ ] CI выполняет lua-тесты.
- [ ] CI выполняет integration-тесты.
- [ ] CI выполняет doctor (там, где позволяет environment).
- [ ] CI matrix включает минимум: Windows MSVC, Windows MinGW, Linux GCC.
- [ ] При наличии разумной инфраструктуры в CI matrix добавлен Clang.