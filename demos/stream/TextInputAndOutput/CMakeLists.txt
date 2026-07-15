# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(text_input_and_output)
target_sources(text_input_and_output PRIVATE
        CaptureReport.cpp
        ConfigureEncoding.cpp
        HandleInvalidEncoding.cpp
        main.cpp
        ReadObservationLines.cpp
        ReadUnicodeBlocks.cpp
        TextInputAndOutputDemos.hpp
        WriteGrowthReport.cpp
)
