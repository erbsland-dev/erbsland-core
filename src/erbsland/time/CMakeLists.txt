# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)
add_subdirectory(tz)

target_sources(erbsland-core PRIVATE
        all.hpp
        CalendarDelta.cpp
        CalendarDelta.hpp
        CalendarParts.hpp
        Date.cpp
        Date.hpp
        DateTime.cpp
        DateTime.hpp
        DateTime_calendar.cpp
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
        Duration_fwd.hpp
        DurationPart.hpp
        ElapsedTimer.hpp
        Hour.hpp
        IsoTimeFormat.hpp
        Literals.hpp
        Minute.hpp
        Month.cpp
        Month.hpp
        Month_fwd.hpp
        Second.hpp
        StdFormat.hpp
        Time.cpp
        Time.hpp
        TimeAmounts.hpp
        TimeDelta.cpp
        TimeDelta.hpp
        TimeDeltaFormat.cpp
        TimeDeltaFormat.hpp
        TimeDeltaUnit.hpp
        TimeOccurrenceInFold.hpp
        TimePoint.hpp
        TimeUnitTags.hpp
        TimeWithZone.cpp
        TimeWithZone.hpp
        TimeZone.cpp
        TimeZone.hpp
        TimeZoneId.hpp
        Year.cpp
        Year.hpp
        Year_fwd.hpp
)
