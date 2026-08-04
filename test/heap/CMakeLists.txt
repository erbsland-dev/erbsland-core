# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_executable(erbsland-core-heap-unittest)
erbsland_set_required_compiler_options(erbsland-core-heap-unittest)
target_link_libraries(erbsland-core-heap-unittest PRIVATE erbsland::core)
target_include_directories(erbsland-core-heap-unittest PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        src
)

add_subdirectory(src)

erbsland_core_configure_unittest(erbsland-core-heap-unittest)
