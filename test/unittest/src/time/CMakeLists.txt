# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        DateTimeTimeZoneTest.cpp
        DateTimeValuesTest.cpp
        DateValuesTest.cpp
        TimeCalendarPartsTest.cpp
        TimeCoreTest.cpp
        TimeDataTestBase.hpp
        TimeModuleTest.cpp
        TimeOffsetTest.cpp
        TimeUnitsTest.cpp
        TimeValuesTest.cpp
        TimeZoneTest.cpp
        TimeZoneTransitionTest.cpp
)
