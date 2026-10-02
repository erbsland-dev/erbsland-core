# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(iso_time_output)
target_sources(iso_time_output PRIVATE
        CompleteOffset.cpp
        Defaults.cpp
        Extended.cpp
        Fractions.cpp
        IsoTimeOutputDemos.hpp
        main.cpp
        OffsetSeconds.cpp
        Precision.cpp
        TimePrefix.cpp
        TimeShift.cpp
)
