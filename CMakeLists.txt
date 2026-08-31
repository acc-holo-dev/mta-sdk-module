cmake_minimum_required(VERSION 3.27)

project(ml_base VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Выбор рантайма для MSVC (CMP0091); на GCC/Clang не влияет.
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

option(SDK_UNITY "Enable unity build" ON)
option(SDK_LTO "Enable link-time optimization" ON)
option(SDK_BUILD_TESTS "Build the embedded Lua test harness (sdk_tests)" ON)
option(SDK_SANITIZE "Build with AddressSanitizer + UndefinedBehaviorSanitizer (GCC/Clang)" OFF)

find_package(Threads REQUIRED)

include(cmake/core/platform.cmake)
include(cmake/core/common.cmake)
include(cmake/lua/lua.cmake)
include(cmake/core/file.cmake)

set(_outdir "${CMAKE_BINARY_DIR}/module/${SDK_PLATFORM_TAG}-${SDK_ARCH_TAG}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${_outdir}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${_outdir}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/.cache/${SDK_PLATFORM_TAG}-${SDK_ARCH_TAG}")

# ---------------------------------------------------------------------------
# Ядро модуля: все исходники, компилируются один раз.
# ---------------------------------------------------------------------------
add_library(sdk_core OBJECT ${SDK_SOURCES})

target_include_directories(sdk_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/mta-sdk
)

target_link_libraries(sdk_core
    PUBLIC
        mta_lua
        Threads::Threads
)

set_target_properties(sdk_core PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

if(SDK_UNITY)
  set_property(TARGET sdk_core PROPERTY UNITY_BUILD ON)
endif()

sdk_target_apply_baseline_options(sdk_core)

# Санитайзеры (ASan/UBSan) — только для отладки, не для продакшн-модуля.
if(SDK_SANITIZE)
  if(MSVC)
    message(WARNING "SDK_SANITIZE поддерживается только на GCC/Clang")
  else()
    foreach(_target sdk_core sdk_tests)
      if(TARGET ${_target})
        target_compile_options(${_target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer -g)
        target_link_options(${_target} PRIVATE -fsanitize=address,undefined)
      endif()
    endforeach()
  endif()
endif()

# ---------------------------------------------------------------------------
# Сам модуль: ml_base.dll / ml_base.so, который загружает MTA-сервер.
# ---------------------------------------------------------------------------
add_library(sdk_base SHARED)
target_link_libraries(sdk_base PRIVATE sdk_core mta_lua)

set_target_properties(sdk_base PROPERTIES
    OUTPUT_NAME "ml_base"
    PREFIX ""
)

sdk_target_apply_baseline_options(sdk_base)
sdk_enable_lto(sdk_base)

if(MSVC)
  include(cmake/config/msvc.cmake)
elseif(MINGW)
  include(cmake/config/mingw.cmake)
elseif(UNIX)
  include(cmake/config/linux.cmake)
endif()

# ---------------------------------------------------------------------------
# Embedded-тест-харнесс: гоняет все функции модуля в чистом Lua 5.1
# с mock-менеджером — сервер MTA не нужен.
# ---------------------------------------------------------------------------
if(SDK_BUILD_TESTS)
  enable_testing()

  add_executable(sdk_tests tests/harness.cpp)
  target_link_libraries(sdk_tests PRIVATE sdk_core mta_lua Threads::Threads)

  target_include_directories(sdk_tests PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${CMAKE_CURRENT_SOURCE_DIR}/vendor/mta-sdk
  )

  target_compile_definitions(sdk_tests PRIVATE
      SDK_TESTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/scripts"
  )

  sdk_target_apply_baseline_options(sdk_tests)
  sdk_enable_lto(sdk_tests)

  add_test(NAME sdk_tests COMMAND sdk_tests)
endif()
