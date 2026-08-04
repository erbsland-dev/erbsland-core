# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        Byte.hpp
        ByteArray.hpp
        ByteArray_fwd.hpp
        ByteBlock.cpp
        ByteBlock.hpp
        ByteBlock_fwd.hpp
        ByteBlockEditor.cpp
        ByteBlockEditor.hpp
        ByteBlockEditor_fwd.hpp
        ByteBuffer.cpp
        ByteBuffer.hpp
        ByteBuffer_fwd.hpp
        ByteIntegerAccess.hpp
        ByteIntegerFormat.hpp
        ByteReader.cpp
        ByteReader.hpp
        ByteReader.tpp
        ByteReader_fwd.hpp
        ByteRingBuffer.hpp
        ByteSpan.cpp
        ByteSpan.hpp
        ByteTextFormat.hpp
        ByteTextOptions.hpp
        ByteTextOptions_fwd.hpp
        ByteWriter.cpp
        ByteWriter.hpp
        ByteWriter.tpp
        ByteWriter_fwd.hpp
        CowManualStorage.hpp
        CowStorage.hpp
        Endianness.hpp
        ReferenceCounter.hpp
        RingBuffer.cpp
        RingBuffer.hpp
        RingBuffer_fwd.hpp
        SecureErase.cpp
        SecureErase.hpp
        SharedArrayData.hpp
        SharedArrayData_fwd.hpp
        SharedArrayDataCleanupMethod.hpp
        SharedArrayDataConstructMethod.hpp
        SharedData.hpp
        SharedDataPointer.hpp
        SharedVirtualData.hpp
        StorageIdentifier.hpp
        UnsafeCharPtr.hpp
        UnsafeMemoryPtr.hpp
)
