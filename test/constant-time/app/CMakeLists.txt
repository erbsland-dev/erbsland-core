# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

# Thin Core Application providing selection, terminal output, lifecycle and interruption handling.
add_executable(erbsland-core-constant-time EXCLUDE_FROM_ALL)
erbsland_core_setup_application(TARGET erbsland-core-constant-time)
erbsland_enable_debug_warnings(erbsland-core-constant-time)
target_link_libraries(erbsland-core-constant-time PRIVATE erbsland-constant-time)

# Listing and deterministic checks remain available in Debug; only optimized builds measure algorithms.
target_compile_definitions(erbsland-core-constant-time PRIVATE
        ERBSLAND_CONSTANT_TIME_BUILD_TYPE="$<CONFIG>"
        ERBSLAND_CONSTANT_TIME_COMPILER="${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}"
        ERBSLAND_CONSTANT_TIME_OPTIMIZED=$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>
)
set_target_properties(erbsland-core-constant-time PROPERTIES
        UNITY_BUILD OFF
        INTERPROCEDURAL_OPTIMIZATION OFF
)

add_subdirectory(src)
