// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Event.hpp"

#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <mutex>

namespace erbsland::event {

class EventDriver;
using EventDriverPtr = std::unique_ptr<EventDriver>;

/// Event driver for creating event-driven applications.
/// All methods in the event driver are thread-safe.
class EventDriver {
public:
    /// The state of the event driver.
    enum class State : uint8_t { Running, Quit };

    /// The result of processing events.
    class ProcessResult final {
    public:
        /// Processing status for one event loop step.
        enum Status : uint8_t { Idle, Processed, Timeout, Quit };
        /// Create one event-loop processing result.
        /// @param status The processing status.
        /// @param exitCode The quit exit code if processing stopped.
        explicit ProcessResult(const Status status = Idle, const int exitCode = 0) noexcept :
            _status{status}, _exitCode{exitCode} {}

    public:
        /// Access the processing status.
        [[nodiscard]] auto status() const noexcept -> Status { return _status; }
        /// Access the quit exit code, meaningful when `status() == Quit`.
        [[nodiscard]] auto exitCode() const noexcept -> int { return _exitCode; }

    private:
        Status _status{Idle}; ///< The processing status.
        int _exitCode{0};     ///< The quit exit code, meaningful when `status == Quit`.
    };

public:
    EventDriver() = default;
    ~EventDriver() = default;
    EventDriver(const EventDriver &) = delete;
    EventDriver(EventDriver &&) = delete;
    auto operator=(const EventDriver &) -> EventDriver & = delete;
    auto operator=(EventDriver &&) -> EventDriver & = delete;

public:
    /// Add an event to the event loop.
    void addEvent(EventIdentifier eventType, EventDataUniquePtr &&eventData);
    /// @overload
    void addEvent(Event &&event);
    /// Set the callback for queued non-control events.
    /// @param eventHandler The handler that receives all non-quit events.
    void setEventHandler(std::function<void(const Event &)> eventHandler);
    /// Test if events are pending in the queue.
    /// @return `true` if at least one event is queued.
    [[nodiscard]] auto hasPendingEvents() const -> bool;

    /// Run this event loop continuously.
    /// This runs the event loop continuously until a quit event is received.
    [[nodiscard]] auto run() -> ProcessResult;

    /// Manually process events with a timeout.
    /// A call of this method will process events in the event queue until the queue is empty or `timeout` is reached;
    /// whatever happens first.
    /// @param timeout The timeout for polling events. Zero = no timeout (not recommended).
    /// @throws std::excepton If an event throws an exception, the exception is rethrown.
    [[nodiscard]] auto processEvents(std::chrono::milliseconds timeout) -> ProcessResult;
    /// Process at most one queued event.
    /// @return `Processed` if one regular event was handled, `Idle` if no event was queued, or `Quit` if the driver
    /// stopped.
    [[nodiscard]] auto processOneEvent() -> ProcessResult;

    /// Process a single event.
    void processEvent(const Event &event);

    /// Quit this event loop with the given exit code
    void quit(int exitCode = 0);

private:
    mutable std::mutex _mutex;                        ///< The mutex for the event queue.
    std::atomic<State> _state{State::Running};        ///< The state
    std::deque<Event> _eventQueue;                    ///< The event queue.
    int _exitCode{0};                                 ///< The exit code of the quit event
    std::function<void(const Event &)> _eventHandler; ///< The callback for non-control events.
};

}
