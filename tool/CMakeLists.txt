# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_library(erbsland-core-tool-lib STATIC)
erbsland_core_setup_static_library(TARGET erbsland-core-tool-lib)
target_include_directories(erbsland-core-tool-lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(erbsland-core-tool)
target_link_libraries(erbsland-core-tool PRIVATE erbsland-core-tool-lib)
erbsland_core_setup_application(TARGET erbsland-core-tool)
add_subdirectory(src)
erbsland_core_add_resources(
        TARGET erbsland-core-tool-lib
        DIRECTORY resources
        IDENTIFIER erbsland-core-tool
        RECURSIVE
        SUFFIXES .cpp .tpl)

if(ERBSLAND_CORE_ENABLE_TESTS)
    add_subdirectory(test/clang-format-fixture)
    add_subdirectory(test/sphinx-fixture)
    add_subdirectory(test/capture-fixture)
    add_subdirectory(test/maintenance-fixture)
    add_subdirectory(test/unittest)
endif()

erbsland_core_add_resources(
        TARGET erbsland-core-tool-lib
        DIRECTORY "${PROJECT_SOURCE_DIR}/cmake/tools/display-text-compiler/data"
        IDENTIFIER erbsland-display-text-compiler
        SUFFIXES .hpp .cpp)
