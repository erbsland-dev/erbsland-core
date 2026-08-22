# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_executable(erbsland-core-resource-integration main.cpp)
erbsland_core_add_resources(
        TARGET erbsland-core-resource-integration
        DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/data"
        IDENTIFIER "fixture"
        RECURSIVE
        SUFFIXES ".txt" ".json"
)
erbsland_core_add_resources(
        TARGET erbsland-core-resource-integration
        DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/raw"
        IDENTIFIER "fixture"
        NO_HASH
        NO_COMPRESSION
)

add_test(
        NAME erbsland-core-resource-integration
        COMMAND $<TARGET_FILE:erbsland-core-resource-integration>
)
