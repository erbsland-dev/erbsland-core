// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IpEndpoint.hpp"
#include "Network_fwd.hpp"
#include "NetworkSourceState.hpp"

#include "../event/EventSource.hpp"

namespace erbsland::network {

/// An inactive TCP listening socket.
/// @notest{Abstract interface; mock and native listener implementations own behavior tests.}
class TcpListener : public event::EventSource {
public:
    // defaults
    ~TcpListener() override = default;

public:
    /// Get the resolved local endpoint.
    /// @return The configured or bound local endpoint.
    [[nodiscard]] virtual auto localEndpoint() const noexcept -> const IpEndpoint & = 0;
    /// Get the source lifecycle state.
    /// @return The current state.
    [[nodiscard]] virtual auto state() const noexcept -> NetworkSourceState = 0;
    /// Start binding and listening.
    virtual void start() = 0;
    /// Suspend delivery of incoming connection requests.
    virtual void pauseAccepting() = 0;
    /// Resume delivery of incoming connection requests.
    virtual void resumeAccepting() = 0;
    /// Close the listener gracefully.
    virtual void close() = 0;
    /// Abort the listener immediately.
    virtual void abort() noexcept = 0;
    /// Create a retained callback editor.
    /// @return A new callback editor connected to this listener.
    [[nodiscard]] virtual auto events() -> TcpListenerEventsPtr = 0;

protected:
    /// Create a listener owned by an event collection.
    /// @param ownerEvents The owner-loop event collection.
    explicit TcpListener(event::EventsPtr ownerEvents) : EventSource{std::move(ownerEvents)} {}
};

}
