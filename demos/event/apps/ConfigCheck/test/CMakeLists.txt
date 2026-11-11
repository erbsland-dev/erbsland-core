# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_executable(erbsland-core-config-check-unittest)
add_subdirectory(src)
target_link_libraries(erbsland-core-config-check-unittest PRIVATE erbsland-core-config-check-support)
erbsland_set_required_compiler_options(erbsland-core-config-check-unittest)
erbsland_core_configure_unittest(erbsland-core-config-check-unittest ENABLE_WARNINGS)
add_test(NAME erbsland-core-config-check-unittest COMMAND erbsland-core-config-check-unittest --no-color)

find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_test(NAME config-check-integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/integration.py" $<TARGET_FILE:config_check>)
set_tests_properties(config-check-integration PROPERTIES TIMEOUT 30)
