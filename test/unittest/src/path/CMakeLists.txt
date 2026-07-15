# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        PathAccessTest.cpp
        PathBackendTestBase.hpp
        PathConstructionTest.cpp
        PathContentTest.cpp
        PathConversionTest.cpp
        PathInfoTest.cpp
        PathModificationTest.cpp
        PathNameToolsTest.cpp
        PathOperationsBackendTest.cpp
        PathOperationsTest.cpp
        PathResolveBackendTest.cpp
        PathTemporaryTest.cpp
        PathTestFixture.hpp
        PathTestHelper.hpp
        PathWalkerTest.cpp
)

if(WIN32)
    target_sources(erbsland-core-unittest PRIVATE
            WindowsPathInfoTest.cpp
            WindowsPathOperationsTest.cpp
            WindowsPathResolveTest.cpp
    )
else()
    target_sources(erbsland-core-unittest PRIVATE
            PosixPathInfoTest.cpp
            PosixPathOperationsTest.cpp
            PosixPathResolveTest.cpp
            PosixPathWalkerTest.cpp
    )
endif()
