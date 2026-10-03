# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(environment)
add_subdirectory(service)
add_subdirectory(signal)
add_subdirectory(subprocess)

target_sources(erbsland-core PRIVATE
        PlatformErrorDiagnostic.cpp
        PlatformErrorDiagnostic.hpp
        PosixErrorContext.cpp
        PosixErrorContext.hpp
        ProcessIdAccess.hpp
        ProcessIdAccess_fwd.hpp
        ProcessInfoData.hpp
        ProcessInfoError.hpp
        SystemBackend.cpp
        SystemBackend.hpp
        SystemBackend_fwd.hpp
        SystemInfoBackend.cpp
        SystemInfoBackend.hpp
        SystemInfoBackendStorage.cpp
        SystemInfoBackendStorage.hpp
        SystemInfoBackend_fwd.hpp
        UserLookupBackend.hpp
        UserLookupBackend_fwd.hpp
)

if (WIN32)
    target_sources(erbsland-core PRIVATE
            WindowsErrorContext.cpp
            WindowsErrorContext.hpp
            WindowsSystemInfoBackend.cpp
            WindowsSystemInfoBackend.hpp
            WindowsUserLookupBackend.cpp
            WindowsUserLookupBackend.hpp
    )
    target_link_libraries(erbsland-core PRIVATE Advapi32)
elseif (APPLE)
    target_sources(erbsland-core PRIVATE
            MacosSystemInfoBackend.cpp
            MacosSystemInfoBackend.hpp
            PosixUserLookupBackend.cpp
            PosixUserLookupBackend.hpp
    )
else ()
    target_sources(erbsland-core PRIVATE
            LinuxSystemInfoBackend.cpp
            LinuxSystemInfoBackend.hpp
            PosixUserLookupBackend.cpp
            PosixUserLookupBackend.hpp
    )
endif ()
