# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        all.hpp
        EnvironmentVariables.cpp
        EnvironmentVariables.hpp
        EnvironmentVariables_fwd.hpp
        FileIdentity.hpp
        GroupId.hpp
        GroupName.cpp
        GroupName.hpp
        PlatformError.cpp
        PlatformError.hpp
        PlatformErrorCategory.cpp
        PlatformErrorCategory.hpp
        PlatformErrorContext.hpp
        PlatformErrorContext_fwd.hpp
        PosixErrorContext.cpp
        PosixErrorContext.hpp
        Subprocess.cpp
        Subprocess.hpp
        Subprocess_fwd.hpp
        SubprocessExitStatus.hpp
        SubprocessOptions.hpp
        SubprocessOptions_fwd.hpp
        SubprocessOutputMode.hpp
        UserId.hpp
        UserLookup.cpp
        UserLookup.hpp
        UserLookup_fwd.hpp
        UserName.cpp
        UserName.hpp
)

if (WIN32)
    target_sources(erbsland-core PRIVATE
            WindowsErrorContext.cpp
            WindowsErrorContext.hpp
    )
endif()
