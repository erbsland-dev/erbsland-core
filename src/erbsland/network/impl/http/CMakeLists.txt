# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(client)
add_subdirectory(codec)
add_subdirectory(cookie)
add_subdirectory(server)
add_subdirectory(static)

target_sources(erbsland-core PRIVATE
        HttpGrammar.cpp
        HttpGrammar.hpp
)
