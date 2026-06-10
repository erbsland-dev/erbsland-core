# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        EventBackend.hpp
        EventBackend_fwd.hpp
        EventId.hpp
        EventIdInfo.hpp
        EventIdRegistry.cpp
        EventIdRegistry.hpp
        EventIdRegistry_fwd.hpp
        EventLoop.hpp
        EventLoop_fwd.hpp
        EventPipe.hpp
        EventPipeReceiver.hpp
        EventPipeSender.hpp
        EventSource.hpp
        EventSource_fwd.hpp
        EventTarget.hpp
        EventTarget_fwd.hpp
        EventThread.hpp
        EventThread_fwd.hpp
)
