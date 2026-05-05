# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        BitmapAccessor.hpp
        BitmapDrawOptionsTest.cpp
        BitmapTest.cpp
        BitmapTestSupport.hpp
        BufferConvenienceTest.cpp
        BufferDrawOptionsTest.cpp
        BufferTest.cpp
        BufferViewTest.cpp
        CursorBufferTest.cpp
        CursorWriterTest.cpp
        CursorWriterTestProbe.hpp
        ReferenceBuffer.hpp
        RemappedBufferTest.cpp
        RemappedBufferTestSupport.hpp
        WriteClippedBufferTest.cpp
)
