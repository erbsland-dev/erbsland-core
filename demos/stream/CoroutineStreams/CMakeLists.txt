# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(coroutine_streams)
target_sources(coroutine_streams PRIVATE
        AwaitByteRead.cpp
        AwaitTextWrite.cpp
        CancelPendingTask.cpp
        CoroutineStreamsDemos.hpp
        HandleCoroutineTimeout.cpp
        main.cpp
        ProcessBlockGenerator.cpp
        ProcessLineGenerator.cpp
        RetainStreamOwnership.cpp
)
