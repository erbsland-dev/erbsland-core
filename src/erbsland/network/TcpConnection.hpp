// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IpEndpoint.hpp"
#include "Network_fwd.hpp"
#include "NetworkSendStatus.hpp"
#include "NetworkSourceState.hpp"
#include "SocketBufferLimits.hpp"

#include "../event/EventSource.hpp"
#include "../mem/ByteBlock.hpp"

namespace erbsland::network {

/// An inactive connected TCP byte stream.
/// @notest{Abstract interface; mock and native connection implementations own behavior tests.}
class TcpConnection : public event::EventSource {
public:
    // defaults
    ~TcpConnection() override = default;

public:
    /// Get the resolved local endpoint.
    /// @return The local endpoint.
    [[nodiscard]] virtual auto localEndpoint() const noexcept -> const IpEndpoint & = 0;
    /// Get the resolved remote endpoint.
    /// @return The connected peer endpoint.
    [[nodiscard]] virtual auto remoteEndpoint() const noexcept -> const IpEndpoint & = 0;
    /// Get the configured queue limits.
    /// @return The send and receive limits.
    [[nodiscard]] virtual auto bufferLimits() const noexcept -> SocketBufferLimits = 0;
    /// Get the source lifecycle state.
    /// @return The current state.
    [[nodiscard]] virtual auto state() const noexcept -> NetworkSourceState = 0;
    /// Start the inactive connected stream.
    virtual void start() = 0;
    /// Atomically submit a complete byte block.
    /// @param data The owned stream data to queue.
    /// @return Whether the block was accepted, back-pressured, or rejected because the stream is closed.
    [[nodiscard]] virtual auto send(mem::ByteBlock data) -> NetworkSendStatus = 0;
    /// Suspend delivery of received data.
    virtual void pauseReceiving() = 0;
    /// Resume delivery of received data.
    virtual void resumeReceiving() = 0;
    /// Close gracefully after accepted output drains.
    virtual void close() = 0;
    /// Abort the connection immediately.
    virtual void abort() noexcept = 0;
    /// Create a retained callback editor.
    /// @return A new callback editor connected to this stream.
    [[nodiscard]] virtual auto events() -> TcpConnectionEventsPtr = 0;

protected:
    /// Create a connection owned by an event collection.
    /// @param ownerEvents The owner-loop event collection.
    explicit TcpConnection(event::EventsPtr ownerEvents) : EventSource{std::move(ownerEvents)} {}
};

}
