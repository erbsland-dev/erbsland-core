# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(byte_compression)
target_sources(byte_compression PRIVATE
        ByteCompressionDemos.hpp
        CompressOneShot.cpp
        CompressStreaming.cpp
        DecompressOneShot.cpp
        DecompressStreaming.cpp
        main.cpp
)
