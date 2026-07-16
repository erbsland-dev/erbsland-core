# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(diagnostics)
add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        CaptureGroup.hpp
        CaptureGroupIndex.hpp
        CaptureRange.hpp
        ErrorCategory.hpp
        Feature.hpp
        Features.hpp
        Flag.hpp
        Flags.hpp
        Input.hpp
        Input16.hpp
        Input32.hpp
        InputBase.hpp
        InputPosition.hpp
        Match.cpp
        Match.hpp
        Match16.cpp
        Match16.hpp
        Match32.cpp
        Match32.hpp
        MatchBase.cpp
        MatchBase.hpp
        RegEx.cpp
        RegEx.hpp
        RegEx_collectAll.cpp
        RegEx_findAll.cpp
        RegEx_findFirst.cpp
        RegEx_fullMatch.cpp
        RegEx_match.cpp
        RegEx_replaceAll.cpp
        RegExError.cpp
        RegExError.hpp
        RegExErrorContext.hpp
        Settings.hpp
)
