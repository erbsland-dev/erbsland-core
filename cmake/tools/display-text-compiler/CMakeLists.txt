# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_executable(erbsland-core-display-text-compiler
        src/CompilerApplication.cpp
        src/CompilerApplication.hpp
        src/DisplayTextCompiler.cpp
        src/DisplayTextCompiler.hpp
        src/DisplayTextOutput.hpp
        src/main.cpp
)
add_executable(erbsland::core-display-text-compiler ALIAS erbsland-core-display-text-compiler)
target_link_libraries(erbsland-core-display-text-compiler PRIVATE erbsland-core)
target_include_directories(erbsland-core-display-text-compiler PRIVATE "${PROJECT_SOURCE_DIR}/src")
erbsland_set_required_compiler_options(erbsland-core-display-text-compiler)
erbsland_enable_debug_warnings(erbsland-core-display-text-compiler)

erbsland_core_add_resources(
        TARGET erbsland-core-display-text-compiler
        DIRECTORY "${PROJECT_SOURCE_DIR}/cmake/tools/display-text-compiler/data"
        IDENTIFIER erbsland-display-text-compiler
        SUFFIXES .hpp .cpp)
