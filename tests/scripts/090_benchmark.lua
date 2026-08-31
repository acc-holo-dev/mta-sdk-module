-- Бенчмарк (информационный, без проверок): скорость вызова функций модуля.

local n = 200000
local acc = 0
local start = os.clock()
for i = 1, n do
    acc = sample_add(acc, 1)
end
local elapsed = os.clock() - start
print(string.format("benchmark: %d вызовов sample_add за %.3f с = %.0f вызовов/с",
                    n, elapsed, n / elapsed))
