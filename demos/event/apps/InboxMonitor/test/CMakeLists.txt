# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_executable(erbsland-core-inbox-monitor-unittest)
add_subdirectory(src)
target_link_libraries(erbsland-core-inbox-monitor-unittest PRIVATE erbsland-core-inbox-monitor-support)
erbsland_set_required_compiler_options(erbsland-core-inbox-monitor-unittest)
erbsland_core_configure_unittest(erbsland-core-inbox-monitor-unittest ENABLE_WARNINGS)
add_test(NAME erbsland-core-inbox-monitor-unittest COMMAND erbsland-core-inbox-monitor-unittest --no-color)

find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_test(NAME inbox-monitor-integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/integration.py" $<TARGET_FILE:inbox-monitor>)
set_tests_properties(inbox-monitor-integration PROPERTIES TIMEOUT 30)
set_tests_properties(erbsland-core-inbox-monitor-unittest PROPERTIES TIMEOUT 20)
