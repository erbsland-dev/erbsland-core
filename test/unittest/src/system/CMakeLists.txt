# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        EnvironmentChangesTest.cpp
        EnvironmentPathsTest.cpp
        EnvironmentVariablesTest.cpp
        ProcessInfoTest.cpp
        SubprocessFailureTest.cpp
        SubprocessOutputTest.cpp
        SystemInfoTest.cpp
        UserLookupTest.cpp
)

if(UNIX)
    target_sources(erbsland-core-unittest PRIVATE
            PosixUserLookupTest.cpp
    )
endif()
