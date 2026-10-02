# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(duration)
target_sources(duration PRIVATE
        Calculate.cpp
        Convert.cpp
        Create.cpp
        DurationDemos.hpp
        main.cpp
        Parts.cpp
        Precise.cpp
)
