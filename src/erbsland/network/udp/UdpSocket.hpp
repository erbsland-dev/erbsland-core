// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UdpDatagram.hpp"
#include "UdpSocket_fwd.hpp"
#include "UdpSocketEventEditor.hpp"
#include "UdpSocketOptions.hpp"

#include "../IpEndpoint.hpp"
#include "../source/NetworkSendStatus.hpp"
#include "../source/NetworkSourceState.hpp"

#include "../../event/EventSource.hpp"

#include <optional>

namespace erbsland::network {

/// An UDP socket.
/// @tested{UdpSocketTest UdpSocketLiveTest}
class UdpSocket : public event::EventSource {
public:
    // defaults
    ~UdpSocket() override = default;

public:
    /// Get the bound local endpoint.
    /// This method is thread-safe.
    /// @return The bound endpoint, or `std::nullopt` before binding succeeds.
    [[nodiscard]] virtual auto localEndpoint() const -> std::optional<IpEndpoint> = 0;
    /// Get the source lifecycle state.
    /// @return The current state.
    [[nodiscard]] virtual auto state() const noexcept -> NetworkSourceState = 0;
    /// Start binding the inactive socket to a complete endpoint.
    /// @param localEndpoint The local address, port, and optional IPv6 scope.
    /// @param options The datagram and queue options captured for this socket.
    /// @throws err::LogicError If called outside the owner event loop or more than once.
    /// @throws err::ParameterError If the endpoint or options are invalid.
    virtual void start(IpEndpoint localEndpoint, UdpSocketOptions options = {}) = 0;
    /// Start with an automatic IPv4 address and port.
    /// @param options The datagram and queue options captured for this socket.
    void start(UdpSocketOptions options = {});
    /// Start with a local address and automatic port.
    /// @param localAddress The local IPv4 or IPv6 address.
    /// @param options The datagram and queue options captured for this socket.
    void start(IpAddress localAddress, UdpSocketOptions options = {});
    /// Start with an IPv4-any address and a local port.
    /// @param localPort The local port, or the automatic port.
    /// @param options The datagram and queue options captured for this socket.
    void start(Port localPort, UdpSocketOptions options = {});
    /// Start with a local address and port.
    /// @param localAddress The local IPv4 or IPv6 address.
    /// @param localPort The local port, or the automatic port.
    /// @param options The datagram and queue options captured for this socket.
    void start(IpAddress localAddress, Port localPort, UdpSocketOptions options = {});
    /// Atomically submit one addressed datagram.
    /// @param datagram The owned payload and destination endpoint.
    /// @return Whether the datagram was accepted, would block, or was rejected because the socket is closed.
    /// Fixed invalid datagrams throw synchronously; operational failures are reported through `onError()`.
    /// @throws err::LogicError If called outside the owner event loop.
    /// @throws err::ParameterError If the destination or payload is invalid for this socket.
    [[nodiscard]] virtual auto send(const UdpDatagram &datagram) -> NetworkSendStatus = 0;
    /// Atomically submit one payload to a destination endpoint.
    /// @param destination The remote destination endpoint.
    /// @param data The owned datagram payload retained on acceptance.
    /// @return Whether the datagram was accepted, would block, or the socket is closed.
    [[nodiscard]] auto send(const IpEndpoint &destination, const mem::ByteBlock &data) -> NetworkSendStatus;
    /// Suspend delivery of received datagrams.
    virtual void pauseReceiving() = 0;
    /// Resume delivery of received datagrams.
    virtual void resumeReceiving() = 0;
    /// Close gracefully after accepted datagrams drain.
    virtual void close() = 0;
    /// Abort the socket immediately.
    virtual void abort() noexcept = 0;
    /// Access the editor for the socket-owned event handlers.
    /// @return The stable source-owned editor.
    [[nodiscard]] auto events() -> UdpSocketEventEditor & override = 0;

protected:
    /// Create a socket owned by an event collection.
    /// @param ownerEvents The owner-loop event collection.
    explicit UdpSocket(event::EventsPtr ownerEvents) : EventSource{std::move(ownerEvents)} {}
};

}
