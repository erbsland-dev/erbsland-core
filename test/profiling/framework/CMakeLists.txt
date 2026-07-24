# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

include(cmake/ErbslandProfiling.cmake)

add_library(erbsland-profiling STATIC)
add_library(erbsland::profiling ALIAS erbsland-profiling)
target_compile_features(erbsland-profiling PUBLIC cxx_std_20)
target_include_directories(erbsland-profiling PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(erbsland-profiling PUBLIC erbsland::core)
erbsland_set_required_compiler_options(erbsland-profiling)
erbsland_enable_debug_warnings(erbsland-profiling)
set_target_properties(erbsland-profiling PROPERTIES
        FOLDER "Tests/Profiling"
)

add_subdirectory(src)
add_subdirectory(test)
