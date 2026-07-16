# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        CategoryContainsTest.cpp
        CategoryTest.cpp
        CharClassTest.cpp
        CharSequenceTest.cpp
        ReCharacterTest.cpp
        ReCharRangeTest.cpp
        RegexpSpecialCategoriesTest.cpp
        ReplacementParserTest.cpp
        ReplacementTest.cpp
        TextAnchorTest.cpp
)
