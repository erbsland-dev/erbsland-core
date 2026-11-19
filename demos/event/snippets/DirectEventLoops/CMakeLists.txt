# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(direct_event_loops)
target_sources(direct_event_loops PRIVATE
        DelayedEntry.cpp
        DirectEventLoopsDemos.hpp
        ReadyEntries.cpp
        main.cpp
)
