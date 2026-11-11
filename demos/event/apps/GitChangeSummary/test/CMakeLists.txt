# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_executable(erbsland-core-git-change-summary-unittest)
add_subdirectory(src)
target_link_libraries(erbsland-core-git-change-summary-unittest PRIVATE erbsland-core-git-change-summary-support)
erbsland_set_required_compiler_options(erbsland-core-git-change-summary-unittest)
erbsland_core_configure_unittest(erbsland-core-git-change-summary-unittest ENABLE_WARNINGS)

add_test(NAME erbsland-core-git-change-summary-unittest COMMAND erbsland-core-git-change-summary-unittest --no-color)

find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_test(NAME git-change-summary-integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/integration.py" $<TARGET_FILE:git_change_summary>)
set_tests_properties(git-change-summary-integration PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
