# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        ByteStreamTest.cpp
        EncodedTextStreamTest.cpp
        StandardStreamsTest.cpp
        StandardTextOutputStreamTest.cpp
        StringBuilderStreamTest.cpp
)

if(WIN32)
    target_sources(erbsland-core-unittest PRIVATE
            WindowsNativeStreamTest.cpp
    )
else()
    target_sources(erbsland-core-unittest PRIVATE
            PosixNativeStreamTest.cpp
    )
endif()
