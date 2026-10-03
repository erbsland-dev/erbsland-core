// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CurrentEventsScope.hpp"
#include "EventLoop_fwd.hpp"

namespace erbsland::event::impl {

/// Restores event-loop dispatch state and thread-local context on every exit path.
/// @tested{EventLoopTest EventLoopOptionsTest ApplicationEventTest}
class EventLoopRunScope final {
public:
    /// Begin dispatch on the calling thread and register its current event target.
    /// @param loop The loop to dispatch for the lifetime of this scope. Must not already be running.
    explicit EventLoopRunScope(EventLoop &loop);
    /// Finish dispatch before restoring the previous thread-local event target.
    ~EventLoopRunScope() noexcept;

    // defaults/deletions
    EventLoopRunScope(const EventLoopRunScope &) = delete;
    EventLoopRunScope(EventLoopRunScope &&) = delete;
    auto operator=(const EventLoopRunScope &) -> EventLoopRunScope & = delete;
    auto operator=(EventLoopRunScope &&) -> EventLoopRunScope & = delete;

private:
    EventLoop &_loop;                ///< Loop whose dispatch state is restored.
    CurrentEventsScope _eventsScope; ///< Scoped thread-local event target.
};

}
