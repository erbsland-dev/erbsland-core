# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(document_api)
add_subdirectory(value_types)

target_sources(erbsland-core-unittest PRIVATE
        DocumentBuilderTest.cpp
        ErrorCategoryTest.cpp
        ErrorTest.cpp
        LocationTest.cpp
        SourceIdentifierTest.cpp
        TestFormatTest.cpp
)
