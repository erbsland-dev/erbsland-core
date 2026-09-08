# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(byte_array)
target_sources(byte_array PRIVATE
        AccessingBytes.cpp
        BorrowingRanges.cpp
        ByteArrayDemos.hpp
        ChangingBytes.cpp
        ChangingRanges.cpp
        CopyingFromSpan.cpp
        GrowingIntoBuffer.cpp
        main.cpp
        MovingBits.cpp
)
