-- Жизненный цикл: пер-ресурсное состояние сбрасывается при остановке ресурса.

test_assert(sample_session_hit() == 1, "first hit")
test_assert(sample_session_hit() == 2, "second hit")

-- имитируем остановку ресурса (ResourceStopping + ResourceStopped)
test_resource_stop()

-- состояние сброшено: счётчик снова с единицы
test_assert(sample_session_hit() == 1, "state reset after resource stop")

-- имитируем рестарт ресурса (повторная регистрация функций)
test_resource_start()
test_assert(sample_session_hit() == 2, "state persists after restart")
