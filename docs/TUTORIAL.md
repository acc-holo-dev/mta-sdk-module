# Туториал: модуль с нуля

Пошаговый путь от пустой папки до работающего модуля, загруженного на
MTA-сервер. Предполагается, что у вас уже есть компилятор (MinGW-w64 или
MSVC на Windows, GCC на Linux), CMake ≥ 3.27 и Ninja.

## Шаг 1. Клонируем основу

```bash
git clone <ваш-репозиторий> my-module
cd my-module
```

## Шаг 2. Собираем как есть

```bash
# Windows (MinGW-w64)
cmake --preset win-mingw
cmake --build --preset win-mingw

# Linux
cmake --preset linux-gcc
cmake --build --preset linux-gcc
```

Результат: `build/win-mingw/module/win-x64/ml_base.dll` (или `.so` на Linux).

## Шаг 3. Проверяем тесты

```bash
ctest --preset win-mingw
```

Должно быть `100% tests passed`. Это значит, что основа работает.

## Шаг 4. Добавляем первую функцию

Создайте файл `src/functions/basics/my_sum.cpp`:

```cpp
#include "registry/registry.hpp"

MTA_LUA_FUNCTION("my_sum", "Складывает два числа.")
{
    auto [a, b] = mta::lua::args<double, double>(L);
    return mta::lua::push_results(L, a + b);
}
```

Пересоберите — функция `my_sum` уже доступна. Никаких правок в CMake или
центральных файлах: сборка подхватывает новые .cpp автоматически.

## Шаг 5. Переименовываем модуль

1. В `src/module/module.cpp` поменяйте `module_details` (имя, автор, версия).
2. В `CMakeLists.txt` поменяйте `OUTPUT_NAME` (имя DLL).
3. Пересоберите.

## Шаг 6. Ставим на сервер

1. Скопируйте `ml_base.dll` в `mods/deathmatch/modules/` сервера.
2. В `mtaserver.conf` добавьте:

```xml
<module src="ml_base"/>
```

3. Перезапустите сервер. В консоли появится:

```
MODULE: Loaded "Base Module" (1.00) by "anon"
```

## Шаг 7. Проверяем в Lua

В любом ресурсе сервера:

```lua
outputChatBox("2 + 3 = " .. my_sum(2, 3))  -- 2 + 3 = 5
```

## Что дальше

- [README.md](../README.md) — все рецепты функций.
- [API.md](API.md) — полный справочник API.
- [GUIDES.md](GUIDES.md) — продвинутые темы (потоки, async, таблицы).
- `src/functions/` — живые примеры каждой возможности.
