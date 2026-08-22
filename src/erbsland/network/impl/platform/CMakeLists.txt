# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        SocketAddress.cpp
        SocketAddress.hpp
)

if (WIN32)
    target_sources(erbsland-core PRIVATE
            WindowsNetworkRuntime.cpp
            WindowsNetworkRuntime.hpp
    )
    target_link_libraries(erbsland-core PRIVATE Ws2_32)
endif ()
