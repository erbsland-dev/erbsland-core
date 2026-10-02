# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(date_time)
target_sources(date_time PRIVATE
        Calculate.cpp
        Compare.cpp
        Create.cpp
        DateTimeDemos.hpp
        Inspect.cpp
        main.cpp
        Recurrence.cpp
        Text.cpp
        Ticks.cpp
        Zones.cpp
)
