# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        ComparisonHelper.hpp
        CoTaskAwaiter.hpp
        CoTaskAwaiterBase.hpp
        CoTaskCancelled.hpp
        CoTaskCompletion.hpp
        CoTaskState.hpp
        CoTaskStateBase.hpp
        CoWorkAwaiter.hpp
        CoWorkerService.cpp
        CoWorkerService.hpp
        CoWorkState.hpp
        EnumFlagsTraits.hpp
        LoopControl.hpp
)
