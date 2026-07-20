# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(diagnostics)
add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        CaptureGroup.hpp
        CaptureGroup_fwd.hpp
        CaptureGroupIndex.hpp
        CaptureRange.hpp
        CaptureRange_fwd.hpp
        CharAndPosition.hpp
        CharAndPosition_fwd.hpp
        ErrorCategory.hpp
        Feature.hpp
        Features.hpp
        Flag.hpp
        Flags.hpp
        Input.hpp
        Input16.hpp
        Input16_fwd.hpp
        Input32.hpp
        Input32_fwd.hpp
        Input_fwd.hpp
        InputBase.hpp
        InputBase_fwd.hpp
        InputPosition.hpp
        Match.cpp
        Match.hpp
        Match16.cpp
        Match16.hpp
        Match16_fwd.hpp
        Match32.cpp
        Match32.hpp
        Match32_fwd.hpp
        Match_fwd.hpp
        MatchBase.cpp
        MatchBase.hpp
        MatchBase_fwd.hpp
        RegEx.cpp
        RegEx.hpp
        RegEx_collectAll.cpp
        RegEx_findAll.cpp
        RegEx_findFirst.cpp
        RegEx_fullMatch.cpp
        RegEx_fwd.hpp
        RegEx_match.cpp
        RegEx_replaceAll.cpp
        RegExError.cpp
        RegExError.hpp
        RegExErrorContext.hpp
        Settings.hpp
        StdFormatForRegEx.hpp
)
