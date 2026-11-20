# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(inbox-monitor PRIVATE
        InboxMonitorApp.cpp
        InboxMonitorApp.hpp
        main.cpp
)
target_sources(erbsland-core-inbox-monitor-support PRIVATE
        InboxEvents.cpp
        InboxEvents.hpp
        InboxWatcher.cpp
        InboxWatcher.hpp
        InboxWatcher_fwd.hpp
)
