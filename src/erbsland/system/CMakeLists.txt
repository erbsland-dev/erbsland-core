# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        all.hpp
        CpuArchitecture.cpp
        CpuArchitecture.hpp
        DetachedSubprocess.cpp
        DetachedSubprocess.hpp
        DetachedSubprocess_fwd.hpp
        EnvironmentChanges.cpp
        EnvironmentChanges.hpp
        EnvironmentChanges_fwd.hpp
        EnvironmentInherit.hpp
        EnvironmentVariables.cpp
        EnvironmentVariables.hpp
        EnvironmentVariables_fwd.hpp
        EnvironmentVariables_path.cpp
        FileIdentity.hpp
        GroupId.hpp
        GroupName.cpp
        GroupName.hpp
        OperatingSystem.cpp
        OperatingSystem.hpp
        PlatformError.cpp
        PlatformError.hpp
        PlatformErrorCategory.cpp
        PlatformErrorCategory.hpp
        PlatformErrorContext.hpp
        PlatformErrorContext_fwd.hpp
        ProcessId.cpp
        ProcessId.hpp
        ProcessId_fwd.hpp
        ProcessInfo.cpp
        ProcessInfo.hpp
        ProcessInfo_fwd.hpp
        Subprocess.cpp
        Subprocess.hpp
        Subprocess_fwd.hpp
        SubprocessEventEditor.cpp
        SubprocessEventEditor.hpp
        SubprocessExitStatus.hpp
        SubprocessOptions.cpp
        SubprocessOptions.hpp
        SubprocessOptions_fwd.hpp
        SubprocessOutputMode.hpp
        System.hpp
        SystemInfo.cpp
        SystemInfo.hpp
        UserId.hpp
        UserLookup.cpp
        UserLookup.hpp
        UserLookup_fwd.hpp
        UserName.cpp
        UserName.hpp
)
