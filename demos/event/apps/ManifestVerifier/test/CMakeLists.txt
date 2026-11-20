# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_test(NAME manifest-verifier-integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/integration.py" $<TARGET_FILE:manifest-verifier>)
set_tests_properties(manifest-verifier-integration PROPERTIES TIMEOUT 30)
