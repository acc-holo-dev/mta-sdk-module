-- Краевые случаи: nil, лишние аргументы, пустые диапазоны, границы.

-- лишние аргументы игнорируются
test_assert(sample_add(1, 2, 3, 4) == 3, "extra arguments ignored")

-- nil вместо числа — понятная ошибка
local ok, err = pcall(sample_add, nil, 1)
test_assert(not ok, "nil for number raises")
test_assert(err and string.find(err, "must be a number", 1, true) ~= nil, "nil error message")

-- диапазон из одного элемента
local single = {sample_range(5, 5)}
test_assert(#single == 1 and single[1] == 5, "single-element range")

-- пустой диапазон (from > to) — ноль результатов
local empty = {sample_range(5, 4)}
test_assert(#empty == 0, "empty range returns nothing")

-- слишком большой диапазон — ошибка
local ok2, err2 = pcall(sample_range, 1, 2000)
test_assert(not ok2, "huge range raises")
test_assert(err2 and string.find(err2, "диапазон", 1, true) ~= nil, "range error message")

-- явный nil в optional → дефолт
test_assert(sample_greet("x", nil) == "привет, x", "explicit nil uses default")
test_assert(sample_tag("x", nil) == "x/none", "explicit nil -> nullopt")

-- пропущенный обязательный аргумент
local ok3, err3 = pcall(sample_minmax, 5)
test_assert(not ok3, "missing required argument raises")
test_assert(err3 and string.find(err3, "got no value", 1, true) ~= nil, "missing arg message")

-- отмена несуществующего таймера
test_assert(sample_timer_cancel(999999) == false, "cancel nonexistent timer returns false")

-- пустой вызов stack_dump
local n = sample_stack_dump()
test_assert(n == 0, "stack_dump with no args returns count 0")
