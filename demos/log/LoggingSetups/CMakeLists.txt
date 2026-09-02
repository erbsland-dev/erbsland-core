# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(logging_setups)
target_sources(logging_setups PRIVATE
        ApplicationConfiguration.cpp
        DeferredConfiguration.cpp
        LastErrorDump.cpp
        LargeLogging.cpp
        LoggingSetups.hpp
        main.cpp
        MediumLogging.cpp
        MinimalLogging.cpp
)
