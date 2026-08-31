// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventLoop_fwd.hpp"
#include "Events_fwd.hpp"
#include "EventThread_fwd.hpp"

namespace erbsland::event {

/// A one-shot thread that owns and runs one event loop.
/// `start()` can succeed only once. `quit()` requests graceful termination, while `join()` only waits and never
/// requests termination itself. Concrete implementations stop and join a started worker before destruction.
/// The final owning pointer must not be released from the event thread itself.
/// @tested{EventThreadTest ApplicationEventTest}
class EventThread {
public:
    // defaults
    virtual ~EventThread() = default;

public:
    /// Start the event loop on a new worker thread.
    /// The call returns after creating the worker; the event loop may not yet report itself as running. After a
    /// successful call, `isStarted()` remains `true` for the lifetime of this object.
    /// @throws err::LogicError If this thread was started before, including after it was joined.
    /// @throws std::system_error If the native worker thread cannot be created.
    virtual void start() = 0;
    /// Request graceful termination of the event loop.
    /// This operation is idempotent, returns without waiting, and can be called before `start()`. Events queued before
    /// the quit request are processed before the loop terminates.
    virtual void quit() noexcept = 0;
    /// Wait until the thread has finished.
    /// This operation does not request termination. It blocks while the worker is running, returns immediately if the
    /// thread was never started or was already joined, and leaves `isStarted()` unchanged. Exceptions escaping the
    /// worker are not transported through this call and terminate the process instead.
    /// @throws err::LogicError If called from this event thread.
    /// @throws std::system_error If the native join operation fails.
    virtual void join() = 0;
    /// Test whether `start()` completed successfully at least once.
    /// @return `true` after the one successful start, including after the worker terminated and was joined.
    [[nodiscard]] virtual auto isStarted() const noexcept -> bool = 0;
    /// Test whether the event loop is currently running on the worker.
    /// @return `true` only while the event loop is executing; this is not a start or completion barrier.
    [[nodiscard]] virtual auto isRunning() const noexcept -> bool = 0;
    /// Access the event loop owned by this thread.
    /// @return A reference that remains valid for the lifetime of this thread object.
    [[nodiscard]] virtual auto eventLoop() noexcept -> EventLoop & = 0;
    /// Access the event-target interface of the owned loop.
    /// @return A shared event target suitable for posting work to this thread.
    [[nodiscard]] virtual auto events() noexcept -> EventsPtr = 0;
};

}
