# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        CoAsyncGeneratorTest.cpp
        CoGeneratorTest.cpp
        ComparisonHelperTest.cpp
        CoTaskTest.cpp
        EnumFlagsTest.cpp
        HashMapTest.cpp
        HashSetTest.cpp
        ListTest.cpp
        MapTest.cpp
        MoveAwareTestCounts.hpp
        MoveAwareTestValue.hpp
        MoveAwareTestValueHash.hpp
        ResultWithDataTest.cpp
        SetTest.cpp
)
