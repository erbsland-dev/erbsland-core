# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(time_with_zone)
target_sources(time_with_zone PRIVATE
        Create.cpp
        Display.cpp
        main.cpp
        Recurrence.cpp
        Resolve.cpp
        TimeWithZoneDemos.hpp
        Transitions.cpp
)
