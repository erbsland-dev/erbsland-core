# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_library(erbsland-core-fuzz-common INTERFACE)
target_include_directories(erbsland-core-fuzz-common INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_subdirectory(src)
