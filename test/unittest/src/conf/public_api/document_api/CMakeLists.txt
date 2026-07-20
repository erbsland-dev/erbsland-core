# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        NamePathLexerTest.cpp
        NamePathTest.cpp
        NameTest.cpp
        NameTypeTest.cpp
        ValueAsMethodsTest.cpp
        ValueChildValueTest.cpp
        ValueGetListTest.cpp
        ValueGetMethodsTest.cpp
        ValueLocationTest.cpp
        ValueMatrixTest.cpp
        ValueNamingTest.cpp
        ValueParentTest.cpp
        ValueTestHelper.hpp
        ValueToTextTest.cpp
        ValueTypeMethodTest.cpp
        ValueTypeTest.cpp
)
