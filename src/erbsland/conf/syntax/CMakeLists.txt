# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        Diagnostic.hpp
        Document.cpp
        Document.hpp
        Document_fwd.hpp
        Format.cpp
        Format.hpp
        FormatResult.hpp
        FormatRule.hpp
        FormatStyle.cpp
        FormatStyle.hpp
        FormatStylePatch.cpp
        FormatStylePatch.hpp
        Formatter.cpp
        Formatter.hpp
        IndentationStyle.hpp
        MatrixStyle.hpp
        Parser.cpp
        Parser.hpp
        PositionMode.hpp
        Selector.cpp
        Selector.hpp
        Statement.hpp
        Token.hpp
        Token_fwd.hpp
        TokenKind.cpp
        TokenKind.hpp
        ValueWrapping.hpp
        VectorStyle.hpp
        WordCase.hpp
        WordSeparator.hpp
)
