// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HostEndpoint.hpp"
#include "Network_fwd.hpp"
#include "NetworkSourceState.hpp"

#include "../event/EventSource.hpp"

namespace erbsland::network {

/// An inactive outgoing TCP connection attempt.
/// @notest{Abstract interface; mock and native connection implementations own behavior tests.}
class TcpConnectionAttempt : public event::EventSource {
public:
    // defaults
    ~TcpConnectionAttempt() override = default;

public:
    /// Get the unresolved remote endpoint.
    /// @return The configured remote endpoint.
    [[nodiscard]] virtual auto remoteEndpoint() const noexcept -> const HostEndpoint & = 0;
    /// Get the source lifecycle state.
    /// @return The current state.
    [[nodiscard]] virtual auto state() const noexcept -> NetworkSourceState = 0;
    /// Start resolution and connection establishment.
    virtual void start() = 0;
    /// Cancel the attempt immediately.
    virtual void cancel() noexcept = 0;
    /// Create a retained callback editor.
    /// @return A new callback editor connected to this attempt.
    [[nodiscard]] virtual auto events() -> TcpConnectionAttemptEventsPtr = 0;

protected:
    /// Create an attempt owned by an event collection.
    /// @param ownerEvents The owner-loop event collection.
    explicit TcpConnectionAttempt(event::EventsPtr ownerEvents) : EventSource{std::move(ownerEvents)} {}
};

}
