# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(readiness_and_timeouts)
target_sources(readiness_and_timeouts PRIVATE
        CheckReadiness.cpp
        main.cpp
        ReadinessAndTimeoutsDemos.hpp
        RetryAtomicWrite.cpp
        RetryTimedOutRead.cpp
        WaitForInput.cpp
)
