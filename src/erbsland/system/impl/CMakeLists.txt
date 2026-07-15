# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        PlatformErrorDiagnostic.cpp
        PlatformErrorDiagnostic.hpp
        UserLookupBackend.hpp
)

if (WIN32)
    target_sources(erbsland-core PRIVATE
            WindowsUserLookupBackend.cpp
            WindowsUserLookupBackend.hpp
    )
    target_link_libraries(erbsland-core PRIVATE Advapi32)
else ()
    target_sources(erbsland-core PRIVATE
            PosixUserLookupBackend.cpp
            PosixUserLookupBackend.hpp
    )
endif ()
