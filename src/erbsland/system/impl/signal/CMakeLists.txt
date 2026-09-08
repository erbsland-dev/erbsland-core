# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        ProcessSignal.hpp
        ProcessSignalBackend.hpp
        ProcessSignalBackend_fwd.hpp
        ProcessSignalDispatcher.cpp
        ProcessSignalDispatcher.hpp
)

if(WIN32)
    target_sources(erbsland-core PRIVATE
            WindowsProcessSignalBackend.cpp
            WindowsProcessSignalBackend.hpp
    )
else()
    target_sources(erbsland-core PRIVATE
            PosixProcessSignalBackend.cpp
            PosixProcessSignalBackend.hpp
    )
endif()
