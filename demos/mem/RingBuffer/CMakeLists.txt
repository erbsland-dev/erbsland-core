# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(ring_buffer)
target_sources(ring_buffer PRIVATE
        CreatingBuffers.cpp
        main.cpp
        ManagingCapacity.cpp
        ReadingAndWritingBytes.cpp
        ReadingAndWritingIntegers.cpp
        RingBufferDemos.hpp
        UnderstandingGrowth.cpp
)
