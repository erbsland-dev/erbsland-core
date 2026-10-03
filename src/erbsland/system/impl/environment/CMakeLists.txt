# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        EnvironmentChangesProcessor.hpp
        EnvironmentChangesProcessor.tpp
        EnvironmentChangesProcessor_fwd.hpp
        EnvironmentPaths.cpp
        EnvironmentPaths.hpp
        EnvironmentVariableBackend.cpp
        EnvironmentVariableBackend.hpp
        EnvironmentVariableBackend_fwd.hpp
        EnvironmentVariableBackend_paths.cpp
)

if (WIN32)
    target_sources(erbsland-core PRIVATE
            WindowsEnvironmentVariableBackend.cpp
            WindowsEnvironmentVariableBackend.hpp
            WindowsEnvironmentVariableBackend_paths.cpp
    )
else ()
    target_sources(erbsland-core PRIVATE
            PosixEnvironmentVariableBackend.cpp
            PosixEnvironmentVariableBackend.hpp
            PosixEnvironmentVariableBackend_paths.cpp
    )
endif ()
