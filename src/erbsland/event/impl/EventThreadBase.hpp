// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../EventLoop_fwd.hpp"
#include "../Events_fwd.hpp"

#include <mutex>
#include <thread>

namespace erbsland::event::impl {

/// Provides the common lifecycle for a dedicated event-loop thread.
/// @notest{Abstract implementation base.}
class EventThreadBase {
public:
    /// Create an event-thread base with an unstarted event loop.
    EventThreadBase();
    /// Stop and join the event thread when necessary.
    virtual ~EventThreadBase() noexcept;

    // defaults
    EventThreadBase(const EventThreadBase &) = delete;
    auto operator=(const EventThreadBase &) -> EventThreadBase & = delete;
    EventThreadBase(EventThreadBase &&) = delete;
    auto operator=(EventThreadBase &&) -> EventThreadBase & = delete;

public:
    /// Start the event-loop thread.
    void start();
    /// Request the event-loop thread to stop.
    void quit() noexcept;
    /// Wait for the event-loop thread to finish.
    void join();
    /// Test if the event-loop thread has been started.
    [[nodiscard]] auto isStarted() const noexcept -> bool;
    /// Test if the event-loop thread is currently running.
    [[nodiscard]] auto isRunning() const noexcept -> bool;
    /// Get the thread's event loop.
    [[nodiscard]] auto eventLoop() -> event::EventLoop &;
    /// Get the event source associated with the event loop.
    [[nodiscard]] auto events() -> event::EventsPtr;

protected:
    /// Shut down the event loop and join the thread.
    void shutdown() noexcept;
    /// Get the event loop when it has been created.
    [[nodiscard]] auto eventLoopPtr() const noexcept -> const event::EventLoopPtr &;

private:
    /// Run the event loop on the worker thread.
    virtual void runEventLoop() = 0;

private:
    event::EventLoopPtr _eventLoop; ///< The event loop running in the thread.
    mutable std::mutex _mutex;      ///< Protects thread state.
    std::thread _thread;            ///< The native worker thread.
    bool _started{false};           ///< True after `start()` was called.
};

}
