# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_executable(erbsland-core-pipe-log-summary-unittest)
add_subdirectory(src)
target_link_libraries(erbsland-core-pipe-log-summary-unittest PRIVATE erbsland-core-pipe-log-summary-support)
erbsland_set_required_compiler_options(erbsland-core-pipe-log-summary-unittest)
erbsland_core_configure_unittest(erbsland-core-pipe-log-summary-unittest ENABLE_WARNINGS)
add_test(NAME erbsland-core-pipe-log-summary-unittest COMMAND erbsland-core-pipe-log-summary-unittest --no-color)

find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_test(NAME pipe-log-summary-integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/integration.py" $<TARGET_FILE:pipe-log-summary>)
set_tests_properties(pipe-log-summary-integration PROPERTIES TIMEOUT 30)
set_tests_properties(erbsland-core-pipe-log-summary-unittest PROPERTIES TIMEOUT 20)
