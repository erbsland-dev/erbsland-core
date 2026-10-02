# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(time_delta_output)
target_sources(time_delta_output PRIVATE
        Calendar.cpp
        FractionDigits.cpp
        Fractions.cpp
        main.cpp
        Precision.cpp
        Presets.cpp
        Reuse.cpp
        SmallestUnit.cpp
        TimeDeltaOutputDemos.hpp
        UnitSeparator.cpp
        UnitStyle.cpp
        ValueSeparator.cpp
)
