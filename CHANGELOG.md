# Changelog

Все заметные изменения проекта документируются здесь. Формат основан на
[Keep a Changelog](https://keepachangelog.com/ru/1.1.0/), версии — на
[Semantic Versioning](https://semver.org/lang/ru/).

## [1.0.0] — 2026-08-30

### Добавлено

- Крепкая основа MTA:SA Lua-модуля: официальный SDK-контракт
  (`ILuaModuleManager10.h`), границы исключений, саморегистрация функций.
- Типизированный биндер: `MTA_LUA_FUNCTION` (тело-стиль) и `MTA_LUA_FUNC`
  (лямбда-стиль), чтение аргументов через `mta::lua::args<...>`.
- Поддержка таблиц (`Argument`/`Table`), `std::optional`, C++-дефолтов,
  `rest_args`, `context`.
- Асинхронный планировщик (воркеры + `DoPulse`), таймеры, стабильные
  Lua-callback'и (`mta::async::Callback`).
- Пер-ресурсное состояние (`mta::resources::Store`) с автоочисткой.
- Логирование (`mta::log`).
- Embedded-Lua тест-харнесс (`sdk_tests`) с mock-менеджером.
- Сборка под Windows (MinGW/MSVC) и Linux (GCC) через CMake-пресеты.
- Документация: `README.md`, `docs/API.md`, `docs/GUIDES.md`.
- CI (GitHub Actions), `.clang-format`, `.editorconfig`, опция санитайзеров.
