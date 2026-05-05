# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_include_directories(erbsland-core-unittest PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(support)
add_subdirectory(api)
