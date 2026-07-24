# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_executable(erbsland-profiling-unittest)
add_subdirectory(src)
target_compile_features(erbsland-profiling-unittest PRIVATE cxx_std_20)
target_link_libraries(erbsland-profiling-unittest PRIVATE erbsland::profiling)
erbsland_set_required_compiler_options(erbsland-profiling-unittest)
erbsland_enable_debug_warnings(erbsland-profiling-unittest)
erbsland_unittest(TARGET erbsland-profiling-unittest ENABLE_WARNINGS)
set_target_properties(erbsland-profiling-unittest PROPERTIES
        FOLDER "Tests/Profiling"
)
