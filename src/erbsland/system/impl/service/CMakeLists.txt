# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        ServiceLifecycle.hpp
        ServiceLifecycle_fwd.hpp
)

if(WIN32)
    target_sources(erbsland-core PRIVATE
            WindowsServiceLifecycle.cpp
            WindowsServiceLifecycle.hpp
    )
else()
    target_sources(erbsland-core PRIVATE
            PosixServiceLifecycle.cpp
            PosixServiceLifecycle.hpp
    )
endif()
