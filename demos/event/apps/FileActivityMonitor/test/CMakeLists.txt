# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_test(NAME file-activity-monitor-integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/integration.py" $<TARGET_FILE:file_activity_monitor>)
set_tests_properties(file-activity-monitor-integration PROPERTIES TIMEOUT 60)
