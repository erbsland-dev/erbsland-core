# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        DiagnosticTest.cpp
        ErrorDocumentBuilderTest.cpp
)

if(WIN32)
    target_sources(erbsland-core-unittest PRIVATE
            WindowsDiagnosticTest.cpp
    )
elseif(UNIX)
    target_sources(erbsland-core-unittest PRIVATE
            PosixDiagnosticTest.cpp
    )
endif()
