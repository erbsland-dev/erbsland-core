# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        Byte.hpp
        ByteBlock.cpp
        ByteBlock.hpp
        ByteBlock_fwd.hpp
        ByteBlockEditor.cpp
        ByteBlockEditor.hpp
        ByteBlockEditor_fwd.hpp
        ByteReader.cpp
        ByteReader.hpp
        ByteRingBuffer.hpp
        ByteWriter.cpp
        ByteWriter.hpp
        ByteWriter_fwd.hpp
        CowManualStorage.hpp
        CowStorage.hpp
        Endianness.hpp
        ReferenceCounter.hpp
        RingBuffer.cpp
        RingBuffer.hpp
        RingBuffer_fwd.hpp
        SharedArrayData.hpp
        SharedArrayData_fwd.hpp
        SharedData.hpp
        SharedDataPointer.hpp
        SharedVirtualData.hpp
        StorageIdentifier.hpp
        UnsafeCharPtr.hpp
        UnsafeMemoryPtr.hpp
)
