# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(api)
add_subdirectory(compiler)
add_subdirectory(diagnostics)
add_subdirectory(engine)
add_subdirectory(parser)
add_subdirectory(text)

target_sources(erbsland-core-unittest PRIVATE
        StringHelper.hpp
        TestHelper.hpp
)
