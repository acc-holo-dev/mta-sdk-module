cmake_minimum_required(VERSION 3.27)

# The project is the SDK template; the produced module binary is named by
# SDK_MODULE_NAME (default "base" -> base.dll / base.so).
project(mta_sdk_module VERSION 1.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Runtime selection for MSVC (CMP0091); no effect on GCC/Clang.
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

option(SDK_UNITY "Enable unity build" ON)
option(SDK_LTO "Enable link-time optimization" ON)
option(SDK_BUILD_TESTS "Build the embedded Lua test harness (sdk_tests)" ON)
option(SDK_SANITIZE "Build with AddressSanitizer + UndefinedBehaviorSanitizer (GCC/Clang)" OFF)

# ---------------------------------------------------------------------------
# Module identity — every knob is a plain CMake cache variable, so naming the
# module never requires touching C++ sources.
#
#   cmake -DSDK_MODULE_NAME=my_mod          -> my_mod.dll / my_mod.so
#   cmake -DSDK_MODULE_TITLE="My Module"    -> console name
#   cmake -DSDK_MODULE_AUTHOR="Jane Doe"    -> console author
# ---------------------------------------------------------------------------
set(SDK_MODULE_NAME "base" CACHE STRING "Output name of the module binary, without extension (base -> base.dll / base.so)")
set(SDK_MODULE_TITLE "Base Module" CACHE STRING "Module display name shown in the server console")
set(SDK_MODULE_AUTHOR "anon" CACHE STRING "Module author shown in the server console")

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
# Module core: every source, compiled once.
# ---------------------------------------------------------------------------
add_library(sdk_core OBJECT ${SDK_SOURCES})

# Single source of truth for the module version: the float reported by
# sample_version / module_details comes from project(VERSION ...) above
# ("1.1.0" -> "1.1"), never from a hard-coded constant in module.cpp.
if(PROJECT_VERSION MATCHES "^([0-9]+)\\.([0-9]+)")
  set(SDK_MODULE_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}")
else()
  set(SDK_MODULE_VERSION "${PROJECT_VERSION}")
endif()
target_compile_definitions(sdk_core PRIVATE
    SDK_MODULE_VERSION="${SDK_MODULE_VERSION}"
    "SDK_MODULE_TITLE=\"${SDK_MODULE_TITLE}\""
    "SDK_MODULE_AUTHOR=\"${SDK_MODULE_AUTHOR}\""
)

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

# Sanitizers (ASan/UBSan) — for debugging only, not for the production module.
if(SDK_SANITIZE)
  if(MSVC)
    message(WARNING "SDK_SANITIZE is supported only on GCC/Clang")
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
# The module itself: ${SDK_MODULE_NAME}.dll / .so, loaded by the MTA server.
# ---------------------------------------------------------------------------
add_library(sdk_base SHARED)
target_link_libraries(sdk_base PRIVATE sdk_core mta_lua)

# Export ONLY the six MTA entry points: everything else (Lua API, runtime
# internals) stays private to the DLL so module symbols cannot collide with
# other modules loaded into the server process.
if(WIN32)
  # The .def is generated from a template so its LIBRARY name follows
  # SDK_MODULE_NAME automatically (it lives out of source in the build dir).
  file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/module")
  configure_file(
      src/module/module.def.in
      "${CMAKE_BINARY_DIR}/module/${SDK_MODULE_NAME}.def"
      @ONLY)
  target_sources(sdk_base PRIVATE "${CMAKE_BINARY_DIR}/module/${SDK_MODULE_NAME}.def")
endif()

set_target_properties(sdk_base PROPERTIES
    OUTPUT_NAME "${SDK_MODULE_NAME}"
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
# Embedded test harness: runs every module function in clean Lua 5.1 with a
# mock manager — no MTA server needed.
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

# Install rules + packaging for releases (see cmake/install.cmake).
include(cmake/install.cmake)
