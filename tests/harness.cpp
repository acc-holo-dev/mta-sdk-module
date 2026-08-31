// Embedded-тест-харнесс.
//
// Загружает ядро модуля в чистый интерпретатор Lua 5.1 вместе с mock-версией
// ILuaModuleManager10 и прогоняет все tests/scripts/*.lua. Модульные функции
// проверяются без запуска MTA-сервера: типы аргументов, перевод ошибок,
// таблицы, доставка асинхронных результатов, таймеры.
//
// Хелперы со стороны Lua:
//   test_assert(условие, сообщение)  — фиксирует успех/провал
//   test_pump(миллисекунды)         — качает DoPulse указанное время

#include "ILuaModuleManager10.h"

#include "lua/common.hpp"
#include "module/module.hpp"

#include <algorithm>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

namespace
{
int g_passed = 0;
int g_failed = 0;
int g_script_errors = 0;

// Mock менеджера: регистрирует функции в тестовый VM, отдаёт фиктивные
// сведения о сервере и «знает» единственный ресурс test_resource.
class MockModuleManager final : public ILuaModuleManager10
{
public:
    lua_State *test_vm = nullptr;
    std::vector<std::string> registered_names;

    static void report(char *buffer, std::size_t size, const char *format, va_list args)
    {
        std::vsnprintf(buffer, size, format, args);
    }

    void ErrorPrintf(const char *format, ...) override
    {
        char buffer[1024];
        va_list args;
        va_start(args, format);
        report(buffer, sizeof(buffer), format, args);
        va_end(args);
        std::printf("  [mock error] %s\n", buffer);
    }

    void DebugPrintf(lua_State *, const char *format, ...) override
    {
        char buffer[1024];
        va_list args;
        va_start(args, format);
        report(buffer, sizeof(buffer), format, args);
        va_end(args);
        std::printf("  [mock debug] %s\n", buffer);
    }

    void Printf(const char *format, ...) override
    {
        char buffer[1024];
        va_list args;
        va_start(args, format);
        report(buffer, sizeof(buffer), format, args);
        va_end(args);
        std::printf("  [mock log] %s\n", buffer);
    }

    bool RegisterFunction(lua_State *lua_vm, const char *function_name, lua_CFunction function) override
    {
        if (lua_vm == nullptr || function_name == nullptr || function == nullptr)
        {
            return false;
        }
        lua_register(lua_vm, function_name, function);
        registered_names.push_back(function_name);
        return true;
    }

    bool GetResourceName(lua_State *, std::string &name) override
    {
        name = "test_resource";
        return true;
    }

    CChecksum GetResourceMetaChecksum(lua_State *) override
    {
        CChecksum checksum{};
        return checksum;
    }

    CChecksum GetResourceFileChecksum(lua_State *, const char *) override
    {
        CChecksum checksum{};
        return checksum;
    }

    unsigned long GetVersion() override { return 0x10005; }
    const char *GetVersionString() override { return "1.6.0-harness"; }
    const char *GetVersionName() override { return "MockModuleManager"; }
    unsigned long GetNetcodeVersion() override { return 42; }
    const char *GetOperatingSystemName() override { return "harness"; }

    lua_State *GetResourceFromName(const char *resource_name) override
    {
        if (resource_name != nullptr && std::strcmp(resource_name, "test_resource") == 0)
        {
            return test_vm;
        }
        return nullptr;
    }

    bool GetResourceName(lua_State *, char *name, std::size_t length) override
    {
        if (name == nullptr || length == 0)
        {
            return false;
        }
        std::snprintf(name, length, "test_resource");
        return true;
    }

    bool GetResourceFilePath(lua_State *, const char *, char *, std::size_t) override
    {
        return false;
    }
};

int test_assert(lua_State *lua_vm)
{
    const bool condition = lua_toboolean(lua_vm, 1) != 0;

    int line = -1;
    lua_Debug debug{};
    if (lua_getstack(lua_vm, 1, &debug) != 0)
    {
        lua_getinfo(lua_vm, "Sl", &debug);
        line = debug.currentline;
    }

    if (condition)
    {
        ++g_passed;
        return 0;
    }

    const char *message = lua_tostring(lua_vm, 2);
    std::printf("  ASSERT FAILED (строка %d): %s\n", line, message ? message : "?");
    ++g_failed;
    return 0;
}

// Качает pulse() до конца таймаута — чтобы фоновые задачи и таймеры
// успели доставить результаты в тестовый VM.
int test_pump(lua_State *lua_vm)
{
    const lua_Integer timeout_ms = luaL_checkinteger(lua_vm, 1);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    while (std::chrono::steady_clock::now() < deadline)
    {
        mta::module::pulse();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    mta::module::pulse();
    return 0;
}

// Имитирует остановку ресурса: ResourceStopping + ResourceStopped.
int test_resource_stop(lua_State *lua_vm)
{
    mta::module::resource_stopping(lua_vm);
    mta::module::resource_stopped(lua_vm);
    return 0;
}

// Имитирует рестарт ресурса: повторная регистрация функций в VM.
int test_resource_start(lua_State *lua_vm)
{
    mta::module::register_functions(lua_vm);
    return 0;
}
} // namespace

int main()
{
    lua_State *lua_vm = luaL_newstate();
    luaL_openlibs(lua_vm);

    MockModuleManager manager;
    manager.test_vm = lua_vm;

    char module_name[MAX_INFO_LENGTH * 2]{};
    char module_author[MAX_INFO_LENGTH * 2]{};
    float module_version = 0.0F;

    if (!mta::module::initialize(&manager, module_name, module_author, &module_version))
    {
        std::printf("FATAL: модулю не удалось инициализироваться\n");
        return 2;
    }

    mta::module::register_functions(lua_vm);

    lua_register(lua_vm, "test_assert", test_assert);
    lua_register(lua_vm, "test_pump", test_pump);
    lua_register(lua_vm, "test_resource_stop", test_resource_stop);
    lua_register(lua_vm, "test_resource_start", test_resource_start);

    std::printf("harness: зарегистрировано функций модуля: %zu\n", manager.registered_names.size());

    std::vector<fs::path> scripts;
    for (const auto &entry : fs::directory_iterator(SDK_TESTS_DIR))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".lua")
        {
            scripts.push_back(entry.path());
        }
    }
    std::sort(scripts.begin(), scripts.end());

    for (const auto &script : scripts)
    {
        std::printf("== %s\n", script.filename().string().c_str());
        if (luaL_dofile(lua_vm, script.string().c_str()) != LUA_OK)
        {
            const char *error = lua_tostring(lua_vm, -1);
            std::printf("  SCRIPT ERROR: %s\n", error ? error : "unknown");
            lua_pop(lua_vm, 1);
            ++g_script_errors;
        }
        lua_gc(lua_vm, LUA_GCCOLLECT, 0);
    }

    mta::module::shutdown();
    lua_close(lua_vm);

    std::printf("\nharness: прошло %d, упало %d, ошибок скриптов %d\n", g_passed, g_failed,
                g_script_errors);
    return (g_failed > 0 || g_script_errors > 0) ? 1 : 0;
}
