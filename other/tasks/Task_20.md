# Task 20 — Release policy, validation и compatibility

**Источник:** PROMT.md — разделы: 36 — Release policy; 37 — Release validation; 38 — Compatibility; фазы: PHASE 14 — Release
**Область задачи:** Release-политика SDK — состав релизных артефактов (только бинарные DLL/SO), валидация release pipeline и разделение понятий SDK version, Module version, ABI version и MTA compatibility version.

---

# 36. Release policy

GitHub Releases НЕ использовать как distribution system полноценного SDK.

Для release artifact оставлять только:

```text
base.dll
```

для Windows и:

```text
base.so
```

для Linux.

То есть release нужен как:

> proof-of-build / proof-of-validity.

Название файла должно соответствовать developer-defined module name.

Пример:

```text
my_module.dll
my_module.so
```

Не добавлять:

```text
sdk_
mta_
holo_
```

автоматически.

# 37. Release validation

Release pipeline должен:

1. build Windows MSVC;
2. build Windows MinGW;
3. build Linux;
4. run tests;
5. run relevant integration tests;
6. verify resulting DLL/SO exists;
7. publish only binary artifact(s).

Если artifact опубликован — pipeline должен гарантировать, что соответствующий build/test job успешно завершился.

# 38. Compatibility

Разделять:

```text
SDK version
Module version
ABI version
MTA compatibility version
```

Не смешивать эти понятия.

При необходимости в binary metadata или diagnostics указывать их отдельно.

---

## Фазы реализации (из раздела 49 PROMT.md)

## PHASE 14 — Release

Release pipeline:

```text
build
test
integration
package
publish DLL/SO
```

Release artifacts:

```text
<module-name>.dll
<module-name>.so
```

---

## Чек-лист соответствия проекта

- [ ] Release pipeline выполняет build для Windows MSVC, Windows MinGW и Linux.
- [ ] Release pipeline выполняет tests и relevant integration tests перед публикацией артефактов.
- [ ] Release pipeline проверяет существование итогового DLL/SO (verify resulting DLL/SO exists).
- [ ] В релиз публикуются только бинарные артефакты `<module-name>.dll` / `<module-name>.so`; полноценный SDK через GitHub Releases не распространяется.
- [ ] Название релизного файла соответствует developer-defined module name (пример: `my_module.dll`, `my_module.so`).
- [ ] Автоматические префиксы `sdk_`, `mta_`, `holo_` не добавляются к имени модуля.
- [ ] Публикация артефакта выполняется только после успешного завершения соответствующего build/test job.
- [ ] Понятия SDK version, Module version, ABI version и MTA compatibility version разделены и не смешиваются; при необходимости они указываются отдельно в binary metadata или diagnostics.
- [ ] Release pipeline реализует шаги build → test → integration → package → publish DLL/SO.