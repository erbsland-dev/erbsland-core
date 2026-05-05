# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        FastRandom.cpp
        FastRandom.hpp
        Random.cpp
        Random.hpp
        SecureRandom.cpp
        SecureRandom.hpp
        ThreadSafeFastRandom.cpp
        ThreadSafeFastRandom.hpp
)

if(WIN32)
    target_link_libraries(erbsland-core PRIVATE bcrypt)
endif()
