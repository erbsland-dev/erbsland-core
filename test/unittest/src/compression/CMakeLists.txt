# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        ByteCompressionTest.cpp
        CodecBufferTest.cpp
        CompressionCodecTest.cpp
        CompressionErrorTest.cpp
        CompressionMalformedTest.cpp
        CompressionRatioTest.cpp
        CompressionStreamingTest.cpp
        ZipArchiveIoTest.cpp
        ZipArchiveTest.cpp
        ZipTestStreams.hpp
        ZstandardInternalTest.cpp
)
