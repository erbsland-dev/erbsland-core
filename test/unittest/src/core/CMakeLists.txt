# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        ApplicationDataAccessorTest.cpp
        ApplicationErrorTest.cpp
        ApplicationEventTest.cpp
        ApplicationInstanceBuilder.hpp
        ApplicationInstanceBuilderBase.hpp
        ApplicationLogTest.cpp
        ApplicationOptionsTest.cpp
        ApplicationPartApplicationTest.cpp
        ApplicationPartManagerTest.cpp
        ApplicationResourceTest.cpp
        ApplicationServiceLifecycleTest.cpp
        ApplicationTerminalTest.cpp
        ApplicationTestScope.cpp
        ApplicationTestScope.hpp
        ApplicationTestScopeManager.hpp
        ApplicationTestScopeTest.cpp
        ApplicationVersionTest.cpp
        IllegalApplicationInstanceAccess.hpp
)

if(UNIX)
    target_sources(erbsland-core-unittest PRIVATE
            PosixApplicationServiceLifecycleTest.cpp
    )
endif()

if(WIN32)
    target_sources(erbsland-core-unittest PRIVATE
            WindowsApplicationServiceLifecycleTest.cpp
    )
endif()
