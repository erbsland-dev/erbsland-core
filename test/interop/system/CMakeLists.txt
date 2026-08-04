# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_executable(erbsland-core-system-interop)
erbsland_set_required_compiler_options(erbsland-core-system-interop)
target_link_libraries(erbsland-core-system-interop PRIVATE erbsland::core)
target_compile_definitions(erbsland-core-system-interop PRIVATE ERBSLAND_UNITTEST_BUILD=1)

add_subdirectory(src)

erbsland_core_configure_unittest(erbsland-core-system-interop)
