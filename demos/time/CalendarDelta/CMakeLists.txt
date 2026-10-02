# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(calendar_delta)
target_sources(calendar_delta PRIVATE
        Apply.cpp
        Boundaries.cpp
        CalendarDeltaDemos.hpp
        Combine.cpp
        Compose.cpp
        Convert.cpp
        main.cpp
        Zones.cpp
)
