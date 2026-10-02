# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(time_literals)
target_sources(time_literals PRIVATE
        Expressions.cpp
        Limits.cpp
        main.cpp
        Scope.cpp
        TimeLiteralsDemos.hpp
        Units.cpp
)
