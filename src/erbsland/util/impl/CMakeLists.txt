# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        CoAsyncGeneratorNextOperation.hpp
        CoAsyncGeneratorPromise.hpp
        CoAsyncGeneratorPromise_fwd.hpp
        CoAsyncGeneratorTransferAwaiter.hpp
        CoGeneratorIterator.hpp
        CoGeneratorPromise.hpp
        CoGeneratorPromise_fwd.hpp
        ComparisonHelper.hpp
        CoTaskAwaiter.hpp
        CoTaskAwaiterBase.hpp
        CoTaskCancelled.hpp
        CoTaskCompletion.hpp
        CoTaskFinalAwaiter.hpp
        CoTaskPromise.hpp
        CoTaskPromise_fwd.hpp
        CoTaskState.hpp
        CoTaskStateBase.hpp
        CoWorkAwaiter.hpp
        CoWorkerService.cpp
        CoWorkerService.hpp
        CoWorkState.hpp
        EnumFlagsTraits.hpp
        LoopControl.hpp
        ResultValue.hpp
        Throw.cpp
        Throw.hpp
)
