// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IpEndpoint.hpp"
#include "Network_fwd.hpp"
#include "NetworkCallbacks.hpp"
#include "NetworkSourceState.hpp"

#include "../event/EventSource.hpp"

namespace erbsland::network {

/// A pending incoming TCP connection decision.
///
/// Concrete implementations reject an undecided request when it is destroyed.
/// @notest{Abstract interface; listener implementations own request behavior tests.}
class TcpConnectionRequest : public event::EventSource {
public:
    // defaults
    ~TcpConnectionRequest() override = default;

public:
    /// Get the resolved remote endpoint.
    /// @return The connecting peer endpoint.
    [[nodiscard]] virtual auto remoteEndpoint() const noexcept -> const IpEndpoint & = 0;
    /// Get the request lifecycle state.
    /// @return The current state.
    [[nodiscard]] virtual auto state() const noexcept -> NetworkSourceState = 0;
    /// Accept the request and configure its inactive connection.
    /// @param ownerEvents The event collection that owns the accepted connection.
    /// @param setupCallback The callback receiving the inactive connection for setup.
    virtual void accept(event::EventsPtr ownerEvents, TcpConnectionFn setupCallback) = 0;
    /// Reject the pending request immediately.
    virtual void reject() noexcept = 0;

protected:
    /// Create a request owned by an event collection.
    /// @param ownerEvents The owner-loop event collection.
    explicit TcpConnectionRequest(event::EventsPtr ownerEvents) : EventSource{std::move(ownerEvents)} {}
};

}
