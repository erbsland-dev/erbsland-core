# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(event_source)
target_sources(event_source PRIVATE
        EventSourceDemos.hpp
        HandlerSetup.cpp
        IllustrationController.hpp
        LifecycleObserver.hpp
        main.cpp
        ObserverLifetime.cpp
        WeakController.cpp
)
