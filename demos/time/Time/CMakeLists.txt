# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(time)
target_sources(time PRIVATE
        Convert.cpp
        Create.cpp
        Inspect.cpp
        main.cpp
        TimeDemos.hpp
        Wrap.cpp
)
