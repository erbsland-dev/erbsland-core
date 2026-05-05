# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(string_format)
target_sources(string_format PRIVATE
        EmptyPlaceholder.cpp
        EscapeAmount.cpp
        EscapeFormat.cpp
        FloatFormats.cpp
        FormattingPatterns.cpp
        IntegerFormats.cpp
        main.cpp
        PositionalPlaceholders.cpp
        StringFormatDemos.hpp
        SupportedTypes.cpp
        TextFormats.cpp
)
