# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        DisplayTextCompilerTest.cpp
        DisplayTextDataTest.cpp
        DisplayTextTestData.hpp
        DisplayTextTestEntry.hpp
        DisplayTextTest.cpp
        DisplayTextTestLayer.hpp
)

# Compile the generator logic against the test library for direct parser/output coverage.
target_sources(erbsland-core-unittest PRIVATE
        ${PROJECT_SOURCE_DIR}/cmake/tools/display-text-compiler/src/DisplayTextCompiler.cpp
)

target_include_directories(erbsland-core-unittest PRIVATE
        ${PROJECT_SOURCE_DIR}/cmake/tools/display-text-compiler/src
)

erbsland_core_add_resources(
        TARGET erbsland-core-unittest
        DIRECTORY "${PROJECT_SOURCE_DIR}/cmake/tools/display-text-compiler/data"
        IDENTIFIER erbsland-display-text-compiler
        SUFFIXES .hpp .cpp)
