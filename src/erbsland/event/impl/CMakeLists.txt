# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        CallbackEventData.hpp
        CurrentEventsScope.cpp
        CurrentEventsScope.hpp
        EventLoop.cpp
        EventLoop.hpp
        EventLoop_fwd.hpp
        EventThreadBase.cpp
        EventThreadBase.hpp
        EventTimer.cpp
        EventTimer.hpp
        EventTimer_fwd.hpp
        ManagedEventThread.cpp
        ManagedEventThread.hpp
        SchedulerBackend.cpp
        SchedulerBackend.hpp
        SchedulerBackend_fwd.hpp
        TimerEventData.hpp
        TimerEventData_fwd.hpp
        UnmanagedEventThread.cpp
        UnmanagedEventThread.hpp
)
