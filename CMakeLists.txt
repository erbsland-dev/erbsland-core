# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

project(erbsland-core LANGUAGES CXX)

# Option if unit tests should be enabled in this build.
option(ERBSLAND_CORE_ENABLE_TESTS "Enable unit tests" ${PROJECT_IS_TOP_LEVEL})
# Option if standalone interoperability tests should be enabled in this build.
option(ERBSLAND_CORE_ENABLE_INTEROP_TESTS "Enable standalone interoperability tests" OFF)
option(
        ERBSLAND_CORE_REQUIRE_EXTERNAL_TLS_TOOLS
        "Require TLS-1.3-capable OpenSSL and curl for network interoperability tests"
        OFF)
# Option if Clang libFuzzer targets should be enabled in this build.
option(ERBSLAND_CORE_ENABLE_FUZZING "Enable Clang libFuzzer targets" OFF)
# Option if demos should be enabled in this build.
option(ERBSLAND_CORE_ENABLE_DEMOS "Enable demos" ${PROJECT_IS_TOP_LEVEL})
# Option if precompiled headers should be enabled in this build.
option(ERBSLAND_CORE_ENABLE_PRECOMPILED_HEADERS "Enable precompiled headers" ${PROJECT_IS_TOP_LEVEL})
# Option if namespaces shall be not flattened. This is mainly relevant for development.
option(ERBSLAND_CORE_DO_NOT_FLATTEN_NS "Do not flatten namespaces" ${PROJECT_IS_TOP_LEVEL})
# Option to create a developer build.
option(ERBSLAND_CORE_DEVELOPER_BUILD "Enables developer build." ${PROJECT_IS_TOP_LEVEL})

# Unity builds substantially reduce clean build times for this library. Enable them for top-level developer builds,
# while preserving an explicit CMAKE_UNITY_BUILD setting and leaving embedded consumers unaffected.
if(PROJECT_IS_TOP_LEVEL
        AND ERBSLAND_CORE_DEVELOPER_BUILD
        AND NOT DEFINED CMAKE_UNITY_BUILD)
    set(CMAKE_UNITY_BUILD ON)
endif()

include(cmake/debug-warnings.cmake)
include(cmake/compiler-options.cmake)
include(cmake/git-version.cmake)

add_library(erbsland-core STATIC)
add_library(erbsland::core ALIAS erbsland-core)
erbsland_set_required_compiler_options(erbsland-core)
erbsland_enable_debug_warnings(erbsland-core)
erbsland_enable_precompiled_headers(erbsland-core
        GNU_EXCLUDED_SOURCES src/erbsland/options/Options.cpp
)

add_subdirectory(src/erbsland)

# Add the include path for integrations and installations
target_include_directories(erbsland-core
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/erbsland
        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

set(_erbsland_core_install_targets erbsland-core)

include(cmake/install.cmake)
include(cmake/tests.cmake)
include(cmake/demos.cmake)

if(ERBSLAND_CORE_ENABLE_FUZZING)
    add_subdirectory(test/fuzzing)
endif()
