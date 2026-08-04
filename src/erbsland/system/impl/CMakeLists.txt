# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        EnvironmentVariableBackend.hpp
        EnvironmentVariableBackend_fwd.hpp
        PlatformErrorDiagnostic.cpp
        PlatformErrorDiagnostic.hpp
        SubprocessBackend.hpp
        SubprocessBackend_fwd.hpp
        UserLookupBackend.hpp
        UserLookupBackend_fwd.hpp
)

if (WIN32)
    target_sources(erbsland-core PRIVATE
            WindowsEnvironmentVariableBackend.cpp
            WindowsEnvironmentVariableBackend.hpp
            WindowsSubprocessBackend.cpp
            WindowsSubprocessBackend.hpp
            WindowsUserLookupBackend.cpp
            WindowsUserLookupBackend.hpp
    )
    target_link_libraries(erbsland-core PRIVATE Advapi32)
else ()
    target_sources(erbsland-core PRIVATE
            PosixEnvironmentVariableBackend.cpp
            PosixEnvironmentVariableBackend.hpp
            PosixSubprocessBackend.cpp
            PosixSubprocessBackend.hpp
            PosixUserLookupBackend.cpp
            PosixUserLookupBackend.hpp
    )
endif ()
