# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        CurrentEvents.cpp
        CurrentEvents.hpp
        Event.hpp
        EventBackend.hpp
        EventBackend_fwd.hpp
        EventBackendId.hpp
        EventBackendIdInfo.hpp
        EventBackendTarget.hpp
        EventBackendTarget_fwd.hpp
        EventCallback.hpp
        EventData.hpp
        EventData_fwd.hpp
        EventEditor.cpp
        EventEditor.hpp
        EventEditor_fwd.hpp
        EventId.hpp
        EventIdInfo.hpp
        EventLoop.cpp
        EventLoop.hpp
        EventLoop_fwd.hpp
        EventLoopDriver.cpp
        EventLoopDriver.hpp
        EventLoopDriver_fwd.hpp
        EventLoopErrorAction.hpp
        EventLoopErrorHandler.hpp
        EventRegistry.cpp
        EventRegistry.hpp
        EventRegistry_fwd.hpp
        Events.hpp
        Events_fwd.hpp
        EventScheduler.hpp
        EventScheduler_fwd.hpp
        EventSource.cpp
        EventSource.hpp
        EventSource_fwd.hpp
        EventSubscription.hpp
        EventSubscription_fwd.hpp
        EventThread.hpp
        EventThread_fwd.hpp
        EventTimer.hpp
        EventTimer_fwd.hpp
        EventTimerMode.hpp
        ManagedEventThread.hpp
        UnmanagedEventThread.cpp
        UnmanagedEventThread.hpp
)
