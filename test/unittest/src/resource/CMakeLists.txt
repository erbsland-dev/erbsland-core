# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        ResourceManagerTest.cpp
)

add_library(erbsland-core-resource-test-fixtures STATIC)
target_link_libraries(erbsland-core-unittest PRIVATE erbsland-core-resource-test-fixtures)
erbsland_core_add_resources(
        TARGET erbsland-core-resource-test-fixtures
        DIRECTORY "${PROJECT_SOURCE_DIR}/test/unittest/data/resource/hashed"
        IDENTIFIER "test"
        SUFFIXES ".txt"
)
erbsland_core_add_resources(
        TARGET erbsland-core-resource-test-fixtures
        DIRECTORY "${PROJECT_SOURCE_DIR}/test/unittest/data/resource/raw"
        IDENTIFIER "test"
        SUFFIXES ".txt"
        NO_HASH
        NO_COMPRESSION
)
