# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(scalar_conversion)
target_sources(scalar_conversion PRIVATE
        BooleanValues.cpp
        FloatingPointValues.cpp
        IntegerValues.cpp
        main.cpp
        ScalarConversionDemos.hpp
)
