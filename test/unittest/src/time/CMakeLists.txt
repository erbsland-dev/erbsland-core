# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        CalendarDeltaTest.cpp
        DateTimeTimeZoneTest.cpp
        DateTimeValuesTest.cpp
        DateValuesTest.cpp
        StdFormatForTimeTest.cpp
        TimeCalendarPartsTest.cpp
        TimeConverterTest.cpp
        TimeCoreTest.cpp
        TimeDataTestBase.hpp
        TimeModuleTest.cpp
        TimeOffsetTest.cpp
        TimeUnitsTest.cpp
        TimeValuesTest.cpp
        TimeWithZoneTest.cpp
        TimeZoneTest.cpp
        TimeZoneTransitionTest.cpp
)

if(WIN32)
    target_sources(erbsland-core-unittest PRIVATE
            WindowsTimeZoneMapTest.cpp
    )
endif()
