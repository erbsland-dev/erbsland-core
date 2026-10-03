# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        WorkerAdmissionError.hpp
        WorkerExecutor.cpp
        WorkerExecutor.hpp
        WorkerExecutor_fwd.hpp
        WorkerExecutorState.hpp
        WorkerExecutorState_fwd.hpp
        WorkerOperation.cpp
        WorkerOperation.hpp
        WorkerReservation.cpp
        WorkerReservation.hpp
        WorkerServices.cpp
        WorkerServices.hpp
)
