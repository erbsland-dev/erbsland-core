// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventLoopDriver_fwd.hpp"

#include "../time/TimeDelta.hpp"

namespace erbsland::event {

/// The native wait and wake driver for an event loop.
///
/// A driver is called only by its owning event loop, except for `wake()`, which is thread-safe. Native event sources
/// use platform-specific implementation interfaces derived from this class.
/// @tested{EventLoopDriverTest EventLoopTest}
class EventLoopDriver {
public:
    // defaults
    virtual ~EventLoopDriver() = default;

public: // factory
    /// Create the default driver for the current platform.
    /// @return A kqueue, epoll, or IOCP based driver.
    [[nodiscard]] static auto createDefault() -> EventLoopDriverPtr;

public: // interface
    /// Wait until the driver is woken or a native source is ready.
    virtual void wait() = 0;
    /// Wait until the driver is woken, a native source is ready, or the maximum wait elapsed.
    /// A zero or negative duration only polls immediately available native events.
    /// @param maximumWait The maximum wait duration.
    virtual void wait(time::TimeDelta maximumWait) = 0;
    /// Wake the driver.
    /// This method is thread-safe.
    virtual void wake() noexcept = 0;
};

}
