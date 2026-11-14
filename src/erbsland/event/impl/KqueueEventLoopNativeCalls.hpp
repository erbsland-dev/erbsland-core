// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <sys/event.h>

namespace erbsland::event::impl {

/// Injectable native operations for the Kqueue event-loop driver.
/// @tested{KqueueEventLoopDriverTest}
class KqueueEventLoopNativeCalls {
public:
    virtual ~KqueueEventLoopNativeCalls() = default;

public: // native operations
    /// Apply kqueue changes without consuming readiness; follows kevent(2) return and errno conventions.
    /// @param queue The kqueue descriptor.
    /// @param changes The filter changes.
    /// @param count The number of changes.
    /// @return Zero on success, or -1 with errno set.
    virtual auto change(int queue, const struct kevent *changes, int count) noexcept -> int;
};

}
