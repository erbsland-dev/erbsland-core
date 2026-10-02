# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

project(erbsland-core LANGUAGES CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Ordinary unit tests are enabled for top-level development builds.
option(ERBSLAND_CORE_ENABLE_TESTS "Enable unit tests" ${PROJECT_IS_TOP_LEVEL})

# Interoperability checks may require external TLS tools; keep them independent of ordinary unit tests.
option(ERBSLAND_CORE_ENABLE_INTEROP_TESTS "Enable standalone interoperability tests" OFF)
option(
        ERBSLAND_CORE_REQUIRE_EXTERNAL_TLS_TOOLS
        "Require TLS-1.3-capable OpenSSL and curl for network interoperability tests"
        OFF
)

# Constant-time assessment is opt-in and its application/tests stay excluded from the default build.
option(ERBSLAND_CORE_ENABLE_CONSTANT_TIME "Enable on-demand constant-time analysis tools" OFF)

# Optional Clang fuzzing targets.
option(ERBSLAND_CORE_ENABLE_FUZZING "Enable Clang libFuzzer targets" OFF)

# Public API examples.
option(ERBSLAND_CORE_ENABLE_DEMOS "Enable demos" ${PROJECT_IS_TOP_LEVEL})

# Developer compile-time optimizations and explicit namespace views.
option(ERBSLAND_CORE_ENABLE_PRECOMPILED_HEADERS "Enable precompiled headers" ${PROJECT_IS_TOP_LEVEL})
option(ERBSLAND_CORE_DO_NOT_FLATTEN_NS "Do not flatten namespaces" ${PROJECT_IS_TOP_LEVEL})
option(ERBSLAND_CORE_DEVELOPER_BUILD "Enables developer build." ${PROJECT_IS_TOP_LEVEL})

# Unity builds substantially reduce clean build times for this library. Enable them for top-level developer builds,
# while preserving an explicit CMAKE_UNITY_BUILD setting and leaving embedded consumers unaffected.
if(PROJECT_IS_TOP_LEVEL
        AND ERBSLAND_CORE_DEVELOPER_BUILD
        AND NOT DEFINED CMAKE_UNITY_BUILD)
    set(CMAKE_UNITY_BUILD ON)
endif()

# Common compiler and version configuration.
include(cmake/debug-warnings.cmake)
include(cmake/compiler-options.cmake)
include(cmake/git-version.cmake)

# Core library target and its public integration alias.
add_library(erbsland-core STATIC)
add_library(erbsland::core ALIAS erbsland-core)
erbsland_set_required_compiler_options(erbsland-core)
erbsland_enable_debug_warnings(erbsland-core)
erbsland_enable_precompiled_headers(erbsland-core
        GNU_EXCLUDED_SOURCES src/erbsland/options/Options.cpp
)

add_subdirectory(src/erbsland)
add_subdirectory(cmake/tools/resource-compiler)

# Export generated public wrappers to consumers; library compilation uses source headers.
target_include_directories(erbsland-core
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/erbsland
        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

set(_erbsland_core_install_targets erbsland-core erbsland-core-resource-compiler)

# Integration, resources, package installation and test helpers.
include(cmake/application.cmake)
include(cmake/resources.cmake)
include(cmake/package.cmake)
include(cmake/install.cmake)
include(cmake/tests.cmake)

# Optional consumer-facing examples.
if(ERBSLAND_CORE_ENABLE_DEMOS)
    add_subdirectory(demos)
endif()

# Only ordinary unit tests enable CTest discovery.
if(ERBSLAND_CORE_ENABLE_TESTS)
    enable_testing()
endif()

# Test infrastructure also hosts independently enabled tools with their own build policy.
if(ERBSLAND_CORE_ENABLE_TESTS
        OR ERBSLAND_CORE_ENABLE_INTEROP_TESTS
        OR ERBSLAND_CORE_ENABLE_FUZZING
        OR ERBSLAND_CORE_ENABLE_CONSTANT_TIME)
    add_subdirectory(test)
endif()
