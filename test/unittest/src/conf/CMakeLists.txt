# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(assignment)
add_subdirectory(char)
add_subdirectory(constants)
add_subdirectory(decoder)
add_subdirectory(lexer)
add_subdirectory(parser)
add_subdirectory(public_api)
add_subdirectory(source)
add_subdirectory(value)
add_subdirectory(vr)

target_sources(erbsland-core-unittest PRIVATE
        ConfTestHelper.hpp
        StdFormatForConfTest.cpp
)
