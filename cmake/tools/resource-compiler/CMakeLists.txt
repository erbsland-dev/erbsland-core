# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_executable(erbsland-core-resource-compiler EXCLUDE_FROM_ALL
        ResourceCompiler.cpp
        ResourceCompiler.hpp
        main.cpp
)
add_executable(erbsland::core-resource-compiler ALIAS erbsland-core-resource-compiler)
target_link_libraries(erbsland-core-resource-compiler PRIVATE erbsland-core)
target_include_directories(erbsland-core-resource-compiler PRIVATE "${PROJECT_SOURCE_DIR}/src")
erbsland_set_required_compiler_options(erbsland-core-resource-compiler)
erbsland_enable_debug_warnings(erbsland-core-resource-compiler)
set_target_properties(erbsland-core-resource-compiler PROPERTIES
        OUTPUT_NAME erbsland-core-resource-compiler
)
