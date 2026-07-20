# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        LexerAdvancedMultiLineRegexTest.cpp
        LexerAdvancedRegexTest.cpp
        LexerAdvancedTimeDeltaTest.cpp
        LexerBasicTest.cpp
        LexerCoreBooleanTest.cpp
        LexerCoreIntegerTest.cpp
        LexerCoreMetaTest.cpp
        LexerCoreTextTest.cpp
        LexerStandardByteCountTest.cpp
        LexerStandardBytesTest.cpp
        LexerStandardCodeTest.cpp
        LexerStandardDateTest.cpp
        LexerStandardDateTimeTest.cpp
        LexerStandardFloatTest.cpp
        LexerStandardMultiLineBytesTest.cpp
        LexerStandardMultiLineCodeTest.cpp
        LexerStandardMultiLineTextTest.cpp
        LexerStandardSectionListTest.cpp
        LexerStandardTextNamesTest.cpp
        LexerStandardTimeTest.cpp
        LexerStandardValueListTest.cpp
        LexerTestHelper.hpp
        LexerTokenTest.cpp
        LexerValueTestHelper.hpp
        NameLexerTest.cpp
        TokenTypeTest.cpp
)
