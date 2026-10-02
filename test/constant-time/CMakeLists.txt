# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

# Keep every component excluded from the default build; explicitly build the application or unit-test target.
# Create the library before its consumers; the application and tests share the same harness and resources.
add_subdirectory(lib EXCLUDE_FROM_ALL)
add_subdirectory(app EXCLUDE_FROM_ALL)
add_subdirectory(test EXCLUDE_FROM_ALL)
