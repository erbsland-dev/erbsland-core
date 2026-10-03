# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        WorkerBulkTask.hpp
        WorkerDelivery.hpp
        WorkerInvokeTraits.hpp
        WorkerPoolState.cpp
        WorkerPoolState.hpp
        WorkerPoolState_events.cpp
        WorkerPoolState_fwd.hpp
        WorkerPoolWake.cpp
        WorkerPoolWake.hpp
        WorkerResult.cpp
        WorkerResult.hpp
        WorkerSingleTask.hpp
        WorkerTask.hpp
        WorkerTask_fwd.hpp
)
