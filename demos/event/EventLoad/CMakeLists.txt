# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(event_load)
target_sources(event_load PRIVATE
        CollectReadings.cpp
        EventLoadDemos.hpp
        main.cpp
        ReadingBatch.cpp
        ReadingBatch.hpp
        ReplaceableUpdates.cpp
)
