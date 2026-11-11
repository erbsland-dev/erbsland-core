// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../EventTarget_fwd.hpp"

namespace erbsland::event::impl {

/// Scoped thread-local registration for a managed event loop.
/// @tested{ApplicationEventTest EventThreadTest}
class CurrentLoopScope final {
public:
    /// Register the current managed event loop for this thread.
    /// @param target The target interface to expose via `currentLoop()`.
    explicit CurrentLoopScope(EventTargetPtr target) noexcept;

    /// Restore the previously current loop target.
    ~CurrentLoopScope() noexcept;

    // defaults/deletions
    CurrentLoopScope(const CurrentLoopScope &) = delete;
    auto operator=(const CurrentLoopScope &) -> CurrentLoopScope & = delete;
    CurrentLoopScope(CurrentLoopScope &&) = delete;
    auto operator=(CurrentLoopScope &&) -> CurrentLoopScope & = delete;

    /// Get the current thread-local target pointer.
    /// @return The weak pointer registered for the current thread.
    [[nodiscard]] static auto currentLoopWeakPtr() noexcept -> EventTargetWeakPtr;

private:
    static thread_local EventTargetWeakPtr _currentLoop; ///< The current thread-local target binding.

    EventTargetWeakPtr _previousLoop;                    ///< The previous thread-local target binding.
};

/// Access the current thread-local target pointer.
/// @return The weak pointer registered for the current thread.
/// @tested{ApplicationEventTest EventThreadTest EventLoopTest}
[[nodiscard]] auto currentLoopWeakPtr() noexcept -> EventTargetWeakPtr;

}
