# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_tool_library(core frame)
add_subdirectory(src)

erbsland_core_add_resources(
        TARGET erbsland-core-tool-core
        DIRECTORY resources
        IDENTIFIER erbsland-core-tool
        RECURSIVE
        SUFFIXES .cpp .tpl)

erbsland_core_add_resources(
        TARGET erbsland-core-tool-core
        DIRECTORY "${PROJECT_SOURCE_DIR}/cmake/tools/display-text-compiler/data"
        IDENTIFIER erbsland-display-text-compiler
        SUFFIXES .hpp .cpp)
