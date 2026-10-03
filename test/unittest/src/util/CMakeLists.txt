# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        CacheTestKey.hpp
        CoAsyncGeneratorTest.cpp
        CoGeneratorTest.cpp
        ComparisonHelperTest.cpp
        CoTaskTest.cpp
        EnumFlagsTest.cpp
        FifoCacheTest.cpp
        HashMapTest.cpp
        HashSetTest.cpp
        ListTest.cpp
        LruCacheTest.cpp
        MapTest.cpp
        MoveAwareTestCounts.hpp
        MoveAwareTestValue.hpp
        MoveAwareTestValueHash.hpp
        ParallelTestWorkTest.cpp
        ResultWithDataTest.cpp
        SetTest.cpp
        StopSourceTest.cpp
        UtilitiesTest.cpp
        WorkerExecutorTest.cpp
        WorkerPoolBulkTest.cpp
        WorkerPoolEventTest.cpp
        WorkerPoolFatalTest.cpp
        WorkerPoolTest.cpp
)
