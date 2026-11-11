# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(event_shutdown)
target_sources(event_shutdown PRIVATE
        CancellableSurvey.cpp
        CancellableSurvey.hpp
        CooperativeCancellation.cpp
        DrainAcceptedWork.cpp
        EventShutdownDemos.hpp
        main.cpp
)
