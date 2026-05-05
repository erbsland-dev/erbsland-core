# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        Event.hpp
        EventData.hpp
        EventDriver.cpp
        EventDriver.hpp
        EventIdentifier.hpp
        EventScheduler.cpp
        EventScheduler.hpp
        EventThread.cpp
        EventThread.hpp
        QuitEvent.hpp
        ScheduledActionRef.hpp
        StopToken.hpp
)
