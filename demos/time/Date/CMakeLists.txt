# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(date)
target_sources(date PRIVATE
        Boundaries.cpp
        Calculate.cpp
        Convert.cpp
        CreateChecked.cpp
        CreateTyped.cpp
        DateDemos.hpp
        Inspect.cpp
        main.cpp
)
