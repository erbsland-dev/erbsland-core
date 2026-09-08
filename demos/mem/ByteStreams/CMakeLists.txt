# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(byte_streams)
target_sources(byte_streams PRIVATE
        BuildRecord.cpp
        ByteStreamsDemos.hpp
        NavigateReader.cpp
        PositionWriter.cpp
        ReadBytes.cpp
        ReadIntegers.cpp
        ReadRecord.cpp
        ReadText.cpp
        StorageManagement.cpp
        WriteBytes.cpp
        WriteIntegers.cpp
        WriteText.cpp
        main.cpp
)
