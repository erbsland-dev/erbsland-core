# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-network-interop PRIVATE
        ${PROJECT_SOURCE_DIR}/test/unittest/src/core/ApplicationTestScope.cpp
        ExternalTlsServerInteropTest.cpp
        HttpHttparseInteropTest.cpp
        HttpClientInteropTest.cpp
        main.cpp
        InteropControl.cpp
        InteropControl.hpp
        InteropEnvironment.cpp
        InteropEnvironment.hpp
        TlsClientConnectionInteropTest.cpp
        TlsServerConnectionInteropTest.cpp
)
