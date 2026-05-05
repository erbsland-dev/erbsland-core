# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(standard_streams)
target_sources(standard_streams PRIVATE
        EasyOutput.cpp
        FloatFormatting.cpp
        IntegerFormatting.cpp
        LowLevelWrite.cpp
        StandardOutputAndError.cpp
        StandardStreamsDemos.hpp
        main.cpp
)
