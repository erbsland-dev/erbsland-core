# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        SubprocessBackend.hpp
        SubprocessBackend_fwd.hpp
        SubprocessErrorContext.cpp
        SubprocessErrorContext.hpp
        SubprocessEventEditor.cpp
        SubprocessEventEditor.hpp
        SubprocessOutputSink.hpp
        SubprocessState.cpp
        SubprocessState.hpp
        SubprocessState_events.cpp
        SubprocessState_fwd.hpp
        SubprocessState_output.cpp
        SubprocessValidation.cpp
        SubprocessValidation.hpp
)

if (WIN32)
    target_sources(erbsland-core PRIVATE
            WindowsSubprocessBackend.cpp
            WindowsSubprocessBackend.hpp
            WindowsSubprocessLaunch.cpp
            WindowsSubprocessLaunch.hpp
    )
else ()
    target_sources(erbsland-core PRIVATE
            PosixSubprocessBackend.cpp
            PosixSubprocessBackend.hpp
            PosixSubprocessLaunch.cpp
            PosixSubprocessLaunch.hpp
    )
endif ()
