# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_executable(erbsland-core-resource-integration main.cpp)
add_library(erbsland-core-resource-static-library STATIC)
erbsland_core_add_resources(
        TARGET erbsland-core-resource-static-library
        DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/data"
        IDENTIFIER "static-fixture"
        SUFFIXES ".txt"
)
add_executable(erbsland-core-resource-static-link empty.cpp)
target_link_libraries(erbsland-core-resource-static-link PRIVATE erbsland-core-resource-static-library)
erbsland_core_add_resources(
        TARGET erbsland-core-resource-integration
        DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/data"
        IDENTIFIER "fixture"
        RECURSIVE
        SUFFIXES ".txt" ".json"
)
target_link_libraries(erbsland-core-resource-integration PRIVATE erbsland-core-resource-static-library)
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
add_test(
        NAME erbsland-core-resource-static-link
        COMMAND $<TARGET_FILE:erbsland-core-resource-static-link>
)
