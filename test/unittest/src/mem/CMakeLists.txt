# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        ByteArrayTest.cpp
        ByteBlockTest.cpp
        ByteBufferTest.cpp
        ByteDataViewTest.cpp
        ByteReaderWriterTest.cpp
        ByteTest.cpp
        CowStorageTest.cpp
        RingBufferTest.cpp
        SecureEraseTest.cpp
        SharedArrayCapacityTest.cpp
        SharedByteDataWithFlagTest.cpp
        SharedDataTest.cpp
)
