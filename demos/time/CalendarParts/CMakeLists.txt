# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(calendar_parts)
target_sources(calendar_parts PRIVATE
        Aggregates.cpp
        Amounts.cpp
        CalendarPartsDemos.hpp
        Construct.cpp
        Lengths.cpp
        main.cpp
        Navigate.cpp
        Ordinal.cpp
        Weekdays.cpp
)
