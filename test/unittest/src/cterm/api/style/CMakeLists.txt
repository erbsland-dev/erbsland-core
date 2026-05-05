# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        Block16StyleTest.cpp
        BlockAttributesTest.cpp
        BlockCombinationStyleTest.cpp
        BlockStyleTest.cpp
        ColorParsingTest.cpp
        ColorSequenceTest.cpp
        ColorTest.cpp
        FrameBorderTest.cpp
        FrameDrawOptionsTest.cpp
        Tile9StyleTest.cpp
        UpdateSettingsTest.cpp
)
