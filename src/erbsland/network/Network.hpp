// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Host.hpp"
#include "HostEndpoint.hpp"
#include "IpEndpoint.hpp"
#include "Network_fwd.hpp"
#include "SocketBufferLimits.hpp"

#include "../event/EventBackendId.hpp"
#include "../event/EventRegistry.hpp"

#include <cstddef>

namespace erbsland::network {

/// The event-loop frontend for asynchronous DNS and socket operations.
/// @notest{Abstract frontend; mock and native backend implementations own behavior tests.}
class Network {
public:
    // defaults
    virtual ~Network() = default;

public:
    /// Get the event-backend identifier for the network frontend.
    /// @return The network backend identifier.
    [[nodiscard]] static constexpr auto backendId() noexcept -> event::EventBackendId {
        return event::id::NetworkBackend;
    }

public: // factories
    /// Create an inactive one-shot host lookup.
    /// @param host The address or unresolved name to resolve.
    /// @return The new lookup source.
    [[nodiscard]] virtual auto createHostLookup(Host host) -> HostLookupPtr = 0;
    /// Create an inactive TCP listener.
    /// @param localEndpoint The local address and port to bind.
    /// @param backlog The requested pending-connection queue length.
    /// @return The new listener source.
    [[nodiscard]] virtual auto createTcpListener(IpEndpoint localEndpoint, std::size_t backlog = 128U)
        -> TcpListenerPtr = 0;
    /// Create an inactive outgoing TCP connection attempt.
    /// @param remoteEndpoint The remote address or host name and port.
    /// @param bufferLimits The send and receive queue limits for the connection.
    /// @return The new connection-attempt source.
    [[nodiscard]] virtual auto createTcpConnection(HostEndpoint remoteEndpoint, SocketBufferLimits bufferLimits = {})
        -> TcpConnectionAttemptPtr = 0;
    /// Create an inactive unconnected UDP socket.
    /// @param localEndpoint The local address and port to bind.
    /// @param bufferLimits The send and receive queue limits.
    /// @return The new UDP socket source.
    [[nodiscard]] virtual auto createUdpSocket(IpEndpoint localEndpoint, SocketBufferLimits bufferLimits = {})
        -> UdpSocketPtr = 0;
    /// Create an inactive UDP source connected to one peer.
    /// @param remoteEndpoint The remote address or host name and port.
    /// @param bufferLimits The send and receive queue limits.
    /// @return The new UDP peer source.
    [[nodiscard]] virtual auto createUdpPeer(HostEndpoint remoteEndpoint, SocketBufferLimits bufferLimits = {})
        -> UdpPeerPtr = 0;
};

}
