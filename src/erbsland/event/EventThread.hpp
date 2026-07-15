// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventLoop_fwd.hpp"
#include "Events_fwd.hpp"
#include "EventThread_fwd.hpp"

namespace erbsland::event {

/// A thread that runs an event loop.
/// @tested{EventThreadTest ApplicationEventTest}
class EventThread {
public:
    virtual ~EventThread() = default;

public:
    /// Start the event loop in a new thread.
    virtual void start() = 0;
    /// Quit the event loop running in this thread.
    virtual void quit() noexcept = 0;
    /// Wait until the thread has finished.
    virtual void join() = 0;
    /// Test if this thread was started.
    [[nodiscard]] virtual auto isStarted() const noexcept -> bool = 0;
    /// Test if the event loop is currently running.
    [[nodiscard]] virtual auto isRunning() const noexcept -> bool = 0;
    /// Access the event loop.
    [[nodiscard]] virtual auto eventLoop() -> EventLoop & = 0;
    /// Access the target interface of the event loop.
    [[nodiscard]] virtual auto events() -> EventsPtr = 0;
};

}
