# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(byte_block)
target_sources(byte_block PRIVATE
        AccessingBytes.cpp
        ByteBlockDemos.hpp
        ComparingBlocks.cpp
        ConvertingBlocks.cpp
        CreatingBlocks.cpp
        EditingBlocks.cpp
        InspectingContents.cpp
        main.cpp
        ManagingStorage.cpp
        ReadingIntegers.cpp
        SlicingBlocks.cpp
)
