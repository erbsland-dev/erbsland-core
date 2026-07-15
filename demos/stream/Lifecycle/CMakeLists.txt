# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(lifecycle)
target_sources(lifecycle PRIVATE
        AbortPendingOutput.cpp
        CloseGracefully.cpp
        ContinueCloseAfterTimeout.cpp
        LifecycleDemos.hpp
        main.cpp
        ObserveFailedState.cpp
)
