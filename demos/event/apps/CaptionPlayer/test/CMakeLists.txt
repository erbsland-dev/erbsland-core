# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_executable(erbsland-core-caption-player-unittest)
add_subdirectory(src)
target_link_libraries(erbsland-core-caption-player-unittest PRIVATE erbsland-core-caption-player-support)
erbsland_set_required_compiler_options(erbsland-core-caption-player-unittest)
erbsland_core_configure_unittest(erbsland-core-caption-player-unittest ENABLE_WARNINGS)
add_test(NAME erbsland-core-caption-player-unittest COMMAND erbsland-core-caption-player-unittest --no-color)

find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_test(NAME caption-player-integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/integration.py" $<TARGET_FILE:caption-player>)
set_tests_properties(caption-player-integration PROPERTIES TIMEOUT 30)
