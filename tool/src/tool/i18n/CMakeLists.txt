# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-tool-lib PRIVATE
        DisplayTextsTool.cpp
        DisplayTextsTool.hpp
)

# Reuse the consumer compiler without its command-line application.
target_sources(erbsland-core-tool-lib PRIVATE
        ${PROJECT_SOURCE_DIR}/cmake/tools/display-text-compiler/src/DisplayTextCompiler.cpp
)
target_include_directories(erbsland-core-tool-lib PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/cmake/tools/display-text-compiler/src
)
