# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        Bzip2BwtResult.hpp
        Bzip2Codec.cpp
        Bzip2Codec.hpp
        Bzip2Crc.cpp
        Bzip2Crc.hpp
        Bzip2Decoder.cpp
        Bzip2Decoder.hpp
        Bzip2Encoder.cpp
        Bzip2Encoder.hpp
        Bzip2HuffmanTable.cpp
        Bzip2HuffmanTable.hpp
        Bzip2MtfResult.hpp
)
