// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventBackend.hpp"
#include "EventBackend_fwd.hpp"
#include "EventCallback.hpp"
#include "EventLoop_fwd.hpp"
#include "EventLoopDriver_fwd.hpp"
#include "EventLoopErrorHandler.hpp"
#include "EventLoopOptions.hpp"
#include "EventTarget.hpp"

#include "../time/TimeDelta.hpp"

#include <exception>

namespace erbsland::event {

/// An event loop.
/// Each event loop owns its queue and backend registry.
/// @seeref{event-loop-options}
/// @tested{ApplicationEventTest EventLoopTest EventThreadTest EventTimerTest EventBackendTest EventLoopOptionsTest}
class EventLoop : public EventTarget {
public:
    // defaults
    ~EventLoop() override = default;

public: // factory methods
    /// Create an event loop with the default backend.
    /// @param options Capacity and default error policies, copied into the loop. Must be valid and must not require
    /// managed-application delivery.
    /// @return The new event loop.
    /// @throws system::PlatformError If a default native driver cannot be created.
    [[nodiscard]] static auto create(const EventLoopOptions &options = {}) -> EventLoopPtr;
    /// Create an event loop with one initial backend.
    /// @param backend The backend to register.
    /// @param options Capacity and default error policies, copied into the loop. Must be valid and must not require
    /// managed-application delivery.
    /// @return The new event loop.
    /// @throws system::PlatformError If a default native driver cannot be created.
    [[nodiscard]] static auto create(EventBackendPtr backend, const EventLoopOptions &options = {}) -> EventLoopPtr;
    /// Create an event loop with a custom native driver.
    /// @param driver The non-null driver to use.
    /// @param options Capacity and default error policies, copied into the loop. Must be valid and must not require
    /// managed-application delivery.
    /// @return The new event loop.
    [[nodiscard]] static auto create(EventLoopDriverPtr driver, const EventLoopOptions &options = {}) -> EventLoopPtr;

public:
    /// Run this event loop until stopped.
    /// @throws system::PlatformError If the native driver wait fails.
    virtual void run() = 0;
    /// Run one event-loop cycle without a maximum wait time.
    /// @return `true` if an event was processed.
    /// @throws system::PlatformError If the native driver wait fails.
    [[nodiscard]] virtual auto runOnce() -> bool = 0;
    /// Run one event-loop cycle with a maximum wait time.
    /// @param maximumWait The maximum time to wait for an event.
    /// @return `true` if an event was processed.
    /// @throws system::PlatformError If the native driver wait fails.
    [[nodiscard]] virtual auto runOnce(time::TimeDelta maximumWait) -> bool = 0;
    /// Run event-loop cycles until there is no immediate work left.
    /// @return The number of processed events.
    /// @throws system::PlatformError If the native driver wait fails.
    [[nodiscard]] virtual auto runUntilIdle() -> std::size_t = 0;
    /// Stop this event loop.
    virtual void stop() noexcept = 0;
    /// Close admission and drain accepted events using reserved quit control.
    virtual void quit() noexcept = 0;
    /// Test if this event loop is running.
    [[nodiscard]] virtual auto isRunning() const noexcept -> bool = 0;
    /// Test if this event loop was requested to quit.
    [[nodiscard]] virtual auto isQuitRequested() const noexcept -> bool = 0;
    /// Get a snapshot of ordinary waiting events, excluding scheduler and shutdown control work.
    [[nodiscard]] virtual auto queuedEventCount() const noexcept -> std::size_t = 0;
    /// Test if a callback exception was captured.
    [[nodiscard]] virtual auto hasError() const noexcept -> bool = 0;
    /// Take the oldest captured callback exception.
    [[nodiscard]] virtual auto takeError() noexcept -> std::exception_ptr = 0;
    /// Set the error handler for callback and backend `err::RuntimeError` exceptions.
    /// @param handler The handler to call after capturing an exception.
    virtual void setErrorHandler(EventLoopErrorHandler handler) = 0;
    /// Register a backend.
    /// @param backend The backend to register.
    virtual void registerBackend(EventBackendPtr backend) = 0;
};

}
