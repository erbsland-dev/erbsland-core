// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpAcceptOptions.hpp"
#include "TcpConnection_fwd.hpp"
#include "TcpConnectionEventEditor.hpp"
#include "TcpConnectionRequest_fwd.hpp"
#include "TcpConnectOptions.hpp"

#include "../HostEndpoint.hpp"
#include "../IpEndpoint.hpp"
#include "../source/NetworkSendStatus.hpp"
#include "../source/NetworkSourceState.hpp"
#include "../source/SocketBufferLimits.hpp"

#include "../../event/EventSource.hpp"
#include "../../mem/ByteBlock.hpp"

namespace erbsland::network {

/// A one-shot TCP connection spanning establishment and active byte-stream use.
/// @notest{Abstract interface; mock and native connection implementations own behavior tests.}
class TcpConnection : public event::EventSource {
public:
    // defaults
    ~TcpConnection() override = default;

public:
    /// Get the resolved local endpoint.
    /// @return The local endpoint.
    [[nodiscard]] virtual auto localEndpoint() const -> std::optional<IpEndpoint> = 0;
    /// Get the resolved remote endpoint.
    /// @return The connected peer endpoint.
    [[nodiscard]] virtual auto remoteEndpoint() const -> std::optional<IpEndpoint> = 0;
    /// Get the configured queue limits.
    /// @return The send and receive limits.
    [[nodiscard]] virtual auto bufferLimits() const noexcept -> SocketBufferLimits = 0;
    /// Get the source lifecycle state.
    /// @return The current state.
    [[nodiscard]] virtual auto state() const noexcept -> NetworkSourceState = 0;
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
    /// Atomically submit a complete byte block.
    /// @param data The owned stream data to queue.
    /// @return Whether the block was accepted, back-pressured, or rejected because the stream is closed.
    [[nodiscard]] virtual auto send(const mem::ByteBlock &data) -> NetworkSendStatus = 0;
    /// Suspend delivery of received data.
    virtual void pauseReceiving() = 0;
    /// Resume delivery of received data.
    virtual void resumeReceiving() = 0;
    /// Close gracefully after accepted output drains.
    virtual void close() = 0;
    /// Abort the connection immediately.
    virtual void abort() noexcept = 0;
    /// Access the editor for the connection-owned event handlers.
    /// @return The stable source-owned editor.
    [[nodiscard]] auto events() -> TcpConnectionEventEditor & override = 0;

protected:
    /// Create a connection owned by an event collection.
    /// @param ownerEvents The owner-loop event collection.
    explicit TcpConnection(event::EventsPtr ownerEvents) : EventSource{std::move(ownerEvents)} {}
};

}
