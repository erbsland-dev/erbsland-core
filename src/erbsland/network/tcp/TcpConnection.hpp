// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpAcceptOptions.hpp"
#include "TcpConnection_fwd.hpp"
#include "TcpConnectionEventEditor.hpp"
#include "TcpConnectionRequest_fwd.hpp"
#include "TcpConnectOptions.hpp"

#include "../HostEndpoint.hpp"
#include "../source/Connection.hpp"

namespace erbsland::network {

/// A one-shot TCP connection spanning establishment and active byte-stream use.
/// @notest{Abstract interface; mock and native connection implementations own behavior tests.}
class TcpConnection : public Connection {
public:
    // defaults
    ~TcpConnection() override = default;

public: // operations
    /// Resolve and connect to a remote endpoint.
    /// @param remoteEndpoint The numeric address or host name and port.
    /// @param options The connection policy and stream limits.
    virtual void connect(HostEndpoint remoteEndpoint, TcpConnectOptions options) = 0;
    /// @overload
    void connect(HostEndpoint remoteEndpoint);
    /// Adopt a pending incoming connection.
    /// @param request The transferable pending request.
    /// @param options The accepted stream options.
    virtual void accept(TcpConnectionRequestPtr request, TcpAcceptOptions options) = 0;
    /// @overload
    void accept(TcpConnectionRequestPtr request);

public: // implement Connection
    [[nodiscard]] auto localEndpoint() const -> std::optional<IpEndpoint> override = 0;
    [[nodiscard]] auto remoteEndpoint() const -> std::optional<IpEndpoint> override = 0;
    [[nodiscard]] auto bufferLimits() const noexcept -> SocketBufferLimits override = 0;
    [[nodiscard]] auto state() const noexcept -> ConnectionState override = 0;
    [[nodiscard]] auto send(const mem::ByteBlock &data) -> NetworkSendStatus override = 0;
    void pauseReceiving() override = 0;
    void resumeReceiving() override = 0;
    void close() override = 0;
    void abort() noexcept override = 0;
    [[nodiscard]] auto events() -> TcpConnectionEventEditor & override = 0;

protected:
    /// Create a connection owned by an event collection.
    /// @param ownerEvents The owner-loop event collection.
    explicit TcpConnection(event::EventsPtr ownerEvents) : Connection{std::move(ownerEvents)} {}
};

}
