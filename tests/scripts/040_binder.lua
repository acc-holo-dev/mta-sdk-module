-- Типизированный биндер: C++-дефолты, optional, кортежи, векторы,
-- вариадика и RAW-режим.

-- дефолт прямо в сигнатуре
test_assert(sample_greet("Вася") == "привет, Вася", "default argument used")
test_assert(sample_greet("Вася", "hi") == "hi, Вася", "default overridden")

-- std::optional
test_assert(sample_tag("x") == "x/none", "optional omitted -> nullopt")
test_assert(sample_tag("x", "t") == "x/t", "optional provided")

-- кортеж из двух значений
local mn, mx = sample_minmax(3, 1)
test_assert(mn == 1 and mx == 3, "pair -> two results")

-- вектор разворачивается в список результатов
local r = {sample_range(2, 5)}
test_assert(#r == 4 and r[1] == 2 and r[4] == 5, "vector -> multiple results")

-- вариадический хвост
local a, b, c = sample_echo(1, "x", true)
test_assert(a == 1 and b == "x" and c == true, "rest_args roundtrip")

-- RAW-режим: прямой стек
local n, t1, t2 = sample_stack_dump(1, "x")
test_assert(n == 2 and t1 == "number" and t2 == "string", "raw stack dump")

-- ошибки типизации прямо из сигнатуры
local ok, err = pcall(sample_greet, {})
test_assert(not ok, "non-string raises")
test_assert(err and string.find(err, "must be a string", 1, true) ~= nil,
            "typed error message from signature")

local ok2, err2 = pcall(sample_range, "x", 1)
test_assert(not ok2 and string.find(err2, "must be an integer", 1, true) ~= nil,
            "integer type error")

-- отсутствующий обязательный аргумент
local ok3, err3 = pcall(sample_minmax, 5)
test_assert(not ok3 and string.find(err3, "got no value", 1, true) ~= nil,
            "missing required argument reports typed error")
