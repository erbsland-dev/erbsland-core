// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <functional>

namespace erbsland::event::impl {

/// A scoped native source registration.
///
/// Destroying or resetting this object unregisters its native source. The driver uses a generation token for every
/// registration, therefore readiness or completion records already queued for an old registration are ignored.
/// @tested{EventLoopDriverTest}
class EventLoopDriverRegistration final {
public:
    /// Create a registration with its unregister operation.
    explicit EventLoopDriverRegistration(std::function<void()> unregister);
    ~EventLoopDriverRegistration();

    // defaults/deletions
    EventLoopDriverRegistration(const EventLoopDriverRegistration &) = delete;
    EventLoopDriverRegistration(EventLoopDriverRegistration &&) = delete;
    auto operator=(const EventLoopDriverRegistration &) -> EventLoopDriverRegistration & = delete;
    auto operator=(EventLoopDriverRegistration &&) -> EventLoopDriverRegistration & = delete;

public:
    /// Unregister the source, if it is still registered.
    void reset() noexcept;
    /// Test if the source is still registered.
    [[nodiscard]] auto isRegistered() const noexcept -> bool;

private:
    std::function<void()> _unregister;
};

}
