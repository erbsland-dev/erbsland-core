# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        Content.hpp
        Core.cpp
        Core.hpp
        Lexer.cpp
        Lexer.hpp
        LexerToken.hpp
        LiteralTables.hpp
        Name.cpp
        Name.hpp
        NameLexer.cpp
        NameLexer.hpp
        Number.cpp
        Number.hpp
        Section.cpp
        Section.hpp
        Text.cpp
        Text.hpp
        TokenGenerator.hpp
        TokenType.hpp
        Value.cpp
        Value.hpp
        ValueBytes.cpp
        ValueBytes.hpp
        ValueDateTime.cpp
        ValueDateTime.hpp
        ValueFloat.cpp
        ValueFloat.hpp
        ValueInteger.cpp
        ValueInteger.hpp
        ValueLiteral.cpp
        ValueLiteral.hpp
        ValueMultiLine.cpp
        ValueMultiLine.hpp
        ValueText.cpp
        ValueText.hpp
)
