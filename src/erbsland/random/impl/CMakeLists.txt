# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        EntropySource.hpp
        PosixEntropySource.hpp
        PosixFileDescriptor.hpp
        SystemEntropySource.cpp
        SystemEntropySource.hpp
        WindowsEntropySource.hpp
)

if(WIN32)
    target_sources(erbsland-core PRIVATE
            SystemEntropySourceWindows.cpp
            WindowsEntropySource.cpp
    )
else()
    target_sources(erbsland-core PRIVATE
            PosixEntropySource.cpp
            PosixFileDescriptor.cpp
            SystemEntropySourcePosix.cpp
    )
endif()
