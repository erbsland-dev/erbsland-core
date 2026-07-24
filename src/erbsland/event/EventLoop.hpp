// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventBackend.hpp"
#include "EventBackend_fwd.hpp"
#include "EventCallback.hpp"
#include "EventLoop_fwd.hpp"
#include "EventLoopDriver_fwd.hpp"
#include "EventLoopErrorHandler.hpp"
#include "Events.hpp"

#include "../time/TimeDelta.hpp"

#include <exception>

namespace erbsland::event {

/// An event loop.
/// Each event loop owns its queue and backend registry.
/// @tested{ApplicationEventTest EventLoopTest EventThreadTest EventTimerTest EventBackendTest}
class EventLoop : public Events {
public:
    ~EventLoop() override = default;

public: // factory methods
    /// Create an event loop with the default backend.
    /// @return The new event loop.
    [[nodiscard]] static auto create() -> EventLoopPtr;
    /// Create an event loop with one initial backend.
    /// @param backend The backend to register.
    /// @return The new event loop.
    [[nodiscard]] static auto create(EventBackendPtr backend) -> EventLoopPtr;
    /// Create an event loop with a custom native driver.
    /// @param driver The non-null driver to use.
    /// @return The new event loop.
    [[nodiscard]] static auto create(EventLoopDriverPtr driver) -> EventLoopPtr;

public:
    /// Run this event loop until stopped.
    virtual void run() = 0;
    /// Run one event-loop cycle without a maximum wait time.
    /// @return `true` if an event was processed.
    [[nodiscard]] virtual auto runOnce() -> bool = 0;
    /// Run one event-loop cycle with a maximum wait time.
    /// @param maximumWait The maximum time to wait for an event.
    /// @return `true` if an event was processed.
    [[nodiscard]] virtual auto runOnce(time::TimeDelta maximumWait) -> bool = 0;
    /// Run event-loop cycles until there is no immediate work left.
    /// @return The number of processed events.
    [[nodiscard]] virtual auto runUntilIdle() -> std::size_t = 0;
    /// Stop this event loop.
    virtual void stop() noexcept = 0;
    /// Quit this event loop by posting a quit event.
    virtual void quit() noexcept = 0;
    /// Test if this event loop is running.
    [[nodiscard]] virtual auto isRunning() const noexcept -> bool = 0;
    /// Test if this event loop was requested to quit.
    [[nodiscard]] virtual auto isQuitRequested() const noexcept -> bool = 0;
    /// Test if a callback exception was captured.
    [[nodiscard]] virtual auto hasError() const noexcept -> bool = 0;
    /// Take the oldest captured callback exception.
    [[nodiscard]] virtual auto takeError() noexcept -> std::exception_ptr = 0;
    /// Set the error handler for callback and backend exceptions.
    /// @param handler The handler to call after capturing an exception.
    virtual void setErrorHandler(EventLoopErrorHandler handler) = 0;
    /// Register a backend.
    /// @param backend The backend to register.
    virtual void registerBackend(EventBackendPtr backend) = 0;
};

}
