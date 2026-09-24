# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_compile_definitions(erbsland-core PRIVATE ERBSLAND_UNITTEST_BUILD=1)

add_subdirectory(compression)
add_subdirectory(network)
add_subdirectory(system)
add_subdirectory(zip)
