// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IpEndpoint.hpp"
#include "Network_fwd.hpp"
#include "NetworkSendStatus.hpp"
#include "NetworkSourceState.hpp"
#include "SocketBufferLimits.hpp"
#include "UdpDatagram.hpp"

#include "../event/EventSource.hpp"

namespace erbsland::network {

/// An inactive unconnected UDP socket.
/// @notest{Abstract interface; mock and native UDP implementations own behavior tests.}
class UdpSocket : public event::EventSource {
public:
    // defaults
    ~UdpSocket() override = default;

public:
    /// Get the resolved local endpoint.
    /// @return The configured or bound local endpoint.
    [[nodiscard]] virtual auto localEndpoint() const noexcept -> const IpEndpoint & = 0;
    /// Get the configured queue limits.
    /// @return The send and receive limits.
    [[nodiscard]] virtual auto bufferLimits() const noexcept -> SocketBufferLimits = 0;
    /// Get the source lifecycle state.
    /// @return The current state.
    [[nodiscard]] virtual auto state() const noexcept -> NetworkSourceState = 0;
    /// Start binding the inactive socket.
    virtual void start() = 0;
    /// Atomically submit one addressed datagram.
    /// @param datagram The owned payload and destination endpoint.
    /// @return Whether the datagram was accepted, back-pressured, or rejected because the socket is closed.
    [[nodiscard]] virtual auto send(UdpDatagram datagram) -> NetworkSendStatus = 0;
    /// Suspend delivery of received datagrams.
    virtual void pauseReceiving() = 0;
    /// Resume delivery of received datagrams.
    virtual void resumeReceiving() = 0;
    /// Close gracefully after accepted datagrams drain.
    virtual void close() = 0;
    /// Abort the socket immediately.
    virtual void abort() noexcept = 0;
    /// Create a retained callback editor.
    /// @return A new callback editor connected to this socket.
    [[nodiscard]] virtual auto events() -> UdpSocketEventsPtr = 0;

protected:
    /// Create a socket owned by an event collection.
    /// @param ownerEvents The owner-loop event collection.
    explicit UdpSocket(event::EventsPtr ownerEvents) : EventSource{std::move(ownerEvents)} {}
};

}
