// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventLoopErrorPolicy.hpp"
#include "EventQueueOverflowAction.hpp"

#include <cstddef>

namespace erbsland::event {

/// Immutable-at-installation capacity and default failure policies for an event loop.
/// @seeref{event-loop-options}
/// @tested{EventLoopOptionsTest}
class EventLoopOptions final {
public: // accessors
    /// Get the ordinary waiting-event limit; zero means unlimited.
    [[nodiscard]] auto maximumQueuedEvents() const noexcept -> std::size_t { return _maximumQueuedEvents; }
    /// Set the ordinary waiting-event limit; zero means unlimited.
    auto setMaximumQueuedEvents(std::size_t value) noexcept -> EventLoopOptions & {
        _maximumQueuedEvents = value;
        return *this;
    }
    /// Get the queue-overflow action.
    [[nodiscard]] auto queueOverflowAction() const noexcept -> EventQueueOverflowAction { return _queueOverflowAction; }
    /// Set the queue-overflow action.
    auto setQueueOverflowAction(EventQueueOverflowAction value) noexcept -> EventLoopOptions & {
        _queueOverflowAction = value;
        return *this;
    }
    /// Get the default `err::RuntimeError` callback policy.
    [[nodiscard]] auto errorPolicy() const noexcept -> EventLoopErrorPolicy { return _errorPolicy; }
    /// Set the default `err::RuntimeError` callback policy.
    auto setErrorPolicy(EventLoopErrorPolicy value) noexcept -> EventLoopOptions & {
        _errorPolicy = value;
        return *this;
    }

private:
    std::size_t _maximumQueuedEvents{0};                                                ///< Zero means unlimited.
    EventQueueOverflowAction _queueOverflowAction{EventQueueOverflowAction::Terminate}; ///< Overflow action.
    EventLoopErrorPolicy _errorPolicy{EventLoopErrorPolicy::Store}; ///< Default callback-error policy.
};

}
