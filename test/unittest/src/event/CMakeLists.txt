# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        EventBackendTest.cpp
        EventLoopDriverTest.cpp
        EventLoopTest.cpp
        EventRegistryTest.cpp
        EventSourceTest.cpp
        EventThreadTest.cpp
        EventTimerTest.cpp
)

if(WIN32)
    target_sources(erbsland-core-unittest PRIVATE
            WindowsEventLoopDriverTest.cpp
    )
elseif(APPLE)
    target_sources(erbsland-core-unittest PRIVATE
            KqueueEventLoopDriverTest.cpp
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_sources(erbsland-core-unittest PRIVATE
            EpollEventLoopDriverTest.cpp
    )
endif()
