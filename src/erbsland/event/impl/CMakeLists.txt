# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        BindEventTraits.hpp
        BoundEvent.hpp
        BoundEvent.tpp
        CallbackEventData.hpp
        CommonEventEditor.cpp
        CommonEventEditor.hpp
        CurrentLoopScope.cpp
        CurrentLoopScope.hpp
        EventCallbackList.hpp
        EventCallbackRegistration.hpp
        EventContinuation.cpp
        EventContinuation.hpp
        EventContinuation_fwd.hpp
        EventLoop.cpp
        EventLoop.hpp
        EventLoop_fwd.hpp
        EventLoop_queue.cpp
        EventLoopDriverRegistration.cpp
        EventLoopDriverRegistration.hpp
        EventLoopRunScope.cpp
        EventLoopRunScope.hpp
        EventMethodHandler.hpp
        EventMethodHandler.tpp
        EventSubscriptionControl.hpp
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
        WeakEventMethodHandler.hpp
        WeakEventMethodHandler.tpp
)

if(WIN32)
    target_sources(erbsland-core PRIVATE
            WindowsEventLoopDriver.cpp
            WindowsEventLoopDriver.hpp
            WindowsEventLoopNativeCalls.cpp
            WindowsEventLoopNativeCalls.hpp
    )
elseif(APPLE)
    target_sources(erbsland-core PRIVATE
            KqueueEventLoopDriver.cpp
            KqueueEventLoopDriver.hpp
            KqueueEventLoopNativeCalls.cpp
            KqueueEventLoopNativeCalls.hpp
    )
elseif(UNIX)
    target_sources(erbsland-core PRIVATE
            EpollEventLoopDriver.cpp
            EpollEventLoopDriver.hpp
            EpollEventLoopNativeCalls.cpp
            EpollEventLoopNativeCalls.hpp
    )
endif()
