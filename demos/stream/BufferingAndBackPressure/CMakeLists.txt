# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(buffering_and_back_pressure)
target_sources(buffering_and_back_pressure PRIVATE
        ApplyBackPressure.cpp
        BufferingAndBackPressureDemos.hpp
        ConfigureBuffers.cpp
        FlushAcceptedOutput.cpp
        main.cpp
        ReadBoundedChunks.cpp
        RejectOversizedRequest.cpp
)
