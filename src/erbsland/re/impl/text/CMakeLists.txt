# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        Category.cpp
        Category.hpp
        Character.cpp
        Character.hpp
        CharClass.cpp
        CharClass.hpp
        CharRange.cpp
        CharRange.hpp
        CharSequence.cpp
        CharSequence.hpp
        Replacement.cpp
        Replacement.hpp
        ReplacementParser.cpp
        ReplacementParser.hpp
        TextAnchor.cpp
        TextAnchor.hpp
)
