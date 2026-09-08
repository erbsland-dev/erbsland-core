# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(service)
add_subdirectory(signal)

target_sources(erbsland-core PRIVATE
        EnvironmentVariableBackend.hpp
        EnvironmentVariableBackend_fwd.hpp
        PlatformErrorDiagnostic.cpp
        PlatformErrorDiagnostic.hpp
        PosixErrorContext.cpp
        PosixErrorContext.hpp
        ProcessIdAccess.hpp
        ProcessIdAccess_fwd.hpp
        ProcessInfoData.hpp
        ProcessInfoError.hpp
        SubprocessBackend.hpp
        SubprocessBackend_fwd.hpp
        SystemInfoBackend.cpp
        SystemInfoBackend.hpp
        SystemInfoBackend_fwd.hpp
        SystemInfoBackendStorage.cpp
        SystemInfoBackendStorage.hpp
        UserLookupBackend.hpp
        UserLookupBackend_fwd.hpp
)

if (WIN32)
    target_sources(erbsland-core PRIVATE
            WindowsEnvironmentVariableBackend.cpp
            WindowsEnvironmentVariableBackend.hpp
            WindowsErrorContext.cpp
            WindowsErrorContext.hpp
            WindowsSystemInfoBackend.cpp
            WindowsSystemInfoBackend.hpp
            WindowsSubprocessBackend.cpp
            WindowsSubprocessBackend.hpp
            WindowsUserLookupBackend.cpp
            WindowsUserLookupBackend.hpp
    )
    target_link_libraries(erbsland-core PRIVATE Advapi32)
elseif (APPLE)
    target_sources(erbsland-core PRIVATE
            MacosSystemInfoBackend.cpp
            MacosSystemInfoBackend.hpp
            PosixEnvironmentVariableBackend.cpp
            PosixEnvironmentVariableBackend.hpp
            PosixSubprocessBackend.cpp
            PosixSubprocessBackend.hpp
            PosixUserLookupBackend.cpp
            PosixUserLookupBackend.hpp
    )
else ()
    target_sources(erbsland-core PRIVATE
            LinuxSystemInfoBackend.cpp
            LinuxSystemInfoBackend.hpp
            PosixEnvironmentVariableBackend.cpp
            PosixEnvironmentVariableBackend.hpp
            PosixSubprocessBackend.cpp
            PosixSubprocessBackend.hpp
            PosixUserLookupBackend.cpp
            PosixUserLookupBackend.hpp
    )
endif ()
