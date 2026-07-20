# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_executable(erbsland-core-test-adapter)
erbsland_set_required_compiler_options(erbsland-core-test-adapter)
target_link_libraries(erbsland-core-test-adapter PRIVATE erbsland::core)

add_subdirectory(src)
