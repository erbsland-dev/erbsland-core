// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HostEndpoint.hpp"
#include "IpEndpoint.hpp"
#include "Network_fwd.hpp"
#include "NetworkSendStatus.hpp"
#include "NetworkSourceState.hpp"
#include "SocketBufferLimits.hpp"

#include "../event/EventSource.hpp"
#include "../mem/ByteBlock.hpp"

#include <optional>

namespace erbsland::network {

/// An inactive UDP socket connected to one peer.
/// @notest{Abstract interface; mock and native UDP implementations own behavior tests.}
class UdpPeer : public event::EventSource {
public:
    // defaults
    ~UdpPeer() override = default;

public:
    /// Get the unresolved remote endpoint.
    /// @return The configured remote host endpoint.
    [[nodiscard]] virtual auto remoteHost() const noexcept -> const HostEndpoint & = 0;
    /// Get the resolved remote endpoint after startup.
    /// @return The resolved endpoint, or `std::nullopt` before resolution completes.
    [[nodiscard]] virtual auto remoteEndpoint() const noexcept -> const std::optional<IpEndpoint> & = 0;
    /// Get the configured queue limits.
    /// @return The send and receive limits.
    [[nodiscard]] virtual auto bufferLimits() const noexcept -> SocketBufferLimits = 0;
    /// Get the source lifecycle state.
    /// @return The current state.
    [[nodiscard]] virtual auto state() const noexcept -> NetworkSourceState = 0;
    /// Start resolution and peer setup.
    virtual void start() = 0;
    /// Atomically submit one datagram payload to the fixed peer.
    /// @param data The owned datagram payload.
    /// @return Whether the payload was accepted, back-pressured, or rejected because the peer is closed.
    [[nodiscard]] virtual auto send(mem::ByteBlock data) -> NetworkSendStatus = 0;
    /// Suspend delivery of received payloads.
    virtual void pauseReceiving() = 0;
    /// Resume delivery of received payloads.
    virtual void resumeReceiving() = 0;
    /// Close gracefully after accepted datagrams drain.
    virtual void close() = 0;
    /// Abort the peer immediately.
    virtual void abort() noexcept = 0;
    /// Create a retained callback editor.
    /// @return A new callback editor connected to this peer.
    [[nodiscard]] virtual auto events() -> UdpPeerEventsPtr = 0;

protected:
    /// Create a peer owned by an event collection.
    /// @param ownerEvents The owner-loop event collection.
    explicit UdpPeer(event::EventsPtr ownerEvents) : EventSource{std::move(ownerEvents)} {}
};

}
