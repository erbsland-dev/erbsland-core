# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(timestamp)
target_sources(timestamp PRIVATE
        Calculate.cpp
        Create.cpp
        DateTime.cpp
        Distances.cpp
        main.cpp
        Storage.cpp
        Text.cpp
        Ticks.cpp
        TimestampDemos.hpp
)
