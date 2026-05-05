# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)
add_subdirectory(tz)

target_sources(erbsland-core PRIVATE
        all.hpp
        CalendarParts.hpp
        Date.cpp
        Date.hpp
        DateTime.cpp
        DateTime.hpp
        DateTimePrecision.hpp
        Day.cpp
        Day.hpp
        DayOfWeek.cpp
        DayOfWeek.hpp
        DayOfWeekFormat.hpp
        DayOfYear.cpp
        DayOfYear.hpp
        DayOfYear_fwd.hpp
        Duration.cpp
        Duration.hpp
        DurationPart.hpp
        ElapsedTimer.hpp
        Hour.hpp
        IsoTimeFormat.hpp
        Minute.hpp
        Month.cpp
        Month.hpp
        Month_fwd.hpp
        Second.hpp
        Time.cpp
        Time.hpp
        TimeAmounts.hpp
        TimeDelta.cpp
        TimeDelta.hpp
        TimeOccurrenceInFold.hpp
        TimePoint.hpp
        TimeUnitTags.hpp
        TimeZone.cpp
        TimeZone.hpp
        TimeZoneId.hpp
        Year.cpp
        Year.hpp
        Year_fwd.hpp
)
