// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Events_fwd.hpp"

namespace erbsland::event::impl {

/// Scoped thread-local registration for a managed event loop.
/// @tested{ApplicationEventTest EventThreadTest}
class CurrentEventsScope final {
public:
    /// Register the current managed event loop for this thread.
    /// @param events The events interface to expose via `currentEvents()`.
    explicit CurrentEventsScope(EventsPtr events) noexcept;

    // defaults
    ~CurrentEventsScope() noexcept;
    CurrentEventsScope(const CurrentEventsScope &) = delete;
    auto operator=(const CurrentEventsScope &) -> CurrentEventsScope & = delete;
    CurrentEventsScope(CurrentEventsScope &&) = delete;
    auto operator=(CurrentEventsScope &&) -> CurrentEventsScope & = delete;

private:
    EventsWeakPtr _previousEvents; ///< The previous thread-local events binding.
};

/// Access the current thread-local events pointer.
/// @return The weak pointer registered for the current thread.
[[nodiscard]] auto currentEventsWeakPtr() noexcept -> EventsWeakPtr;

}
