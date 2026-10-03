// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Network_fwd.hpp"

#include "host_lookup/HostLookup_fwd.hpp"
#include "http_client/HttpClientSession_fwd.hpp"
#include "http_server/HttpServer_fwd.hpp"
#include "tcp/TcpConnection_fwd.hpp"
#include "tcp/TcpListener_fwd.hpp"
#include "tls/TlsClientConnection_fwd.hpp"
#include "tls/TlsServerConnection_fwd.hpp"
#include "udp/UdpSocket_fwd.hpp"

#include "../event/EventBackendId.hpp"
#include "../event/EventRegistry.hpp"

namespace erbsland::network {

/// The event-loop frontend for asynchronous DNS and socket operations.
/// @tested{NetworkFacadeTest NetworkProtocolFacadeTest TlsClientConnectionTest TcpSocketLiveTest UdpSocketLiveTest}
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
    /// Create an inactive reusable host lookup.
    /// Must be called on the owner event loop.
    /// @return The new lookup source.
    [[nodiscard]] virtual auto createHostLookup() -> HostLookupPtr = 0;
    /// Create an active session-first HTTP/1.1 and HTTPS client.
    /// Must be called on the owner event loop.
    /// @return The new client session.
    [[nodiscard]] virtual auto createHttpClientSession() -> HttpClientSessionPtr = 0;
    /// Create an inactive HTTP/1.1 or HTTPS server.
    /// Must be called on the owner event loop.
    /// @return The new server source.
    [[nodiscard]] virtual auto createHttpServer() -> HttpServerPtr = 0;
    /// Create an inactive TCP listener.
    /// @return The new listener source.
    [[nodiscard]] virtual auto createTcpListener() -> TcpListenerPtr = 0;
    /// Create an inactive TCP connection.
    /// @return The new connection source.
    [[nodiscard]] virtual auto createTcpConnection() -> TcpConnectionPtr = 0;
    /// Create an inactive TLS client connection.
    /// Must be called on the owner event loop.
    /// @return The new one-shot authenticated connection source.
    [[nodiscard]] virtual auto createTlsClientConnection() -> TlsClientConnectionPtr = 0;
    /// Create an inactive accepted TLS server connection.
    /// Must be called on the owner event loop.
    /// @return The new one-shot authenticated server connection source.
    [[nodiscard]] virtual auto createTlsServerConnection() -> TlsServerConnectionPtr = 0;
    /// Create an inactive UDP socket.
    /// @return The new UDP socket source.
    [[nodiscard]] virtual auto createUdpSocket() -> UdpSocketPtr = 0;
};

}
