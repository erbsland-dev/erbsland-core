// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Connection_fwd.hpp"
#include "ConnectionEventEditor.hpp"
#include "ConnectionState.hpp"
#include "NetworkSendStatus.hpp"
#include "SocketBufferLimits.hpp"

#include "../impl/ConnectionProtocolAccess_fwd.hpp"
#include "../IpEndpoint.hpp"

#include "../../event/EventSource.hpp"
#include "../../mem/ByteBlock.hpp"

#include <memory>
#include <optional>
#include <utility>

namespace erbsland::network {

/// A one-shot event-driven application byte-stream connection.
/// Protocol-specific subclasses perform their own setup before entering `ConnectionState::Active`.
/// @tested{ConnectionTest TcpConnectionTest TlsClientConnectionTest TlsServerConnectionTest}
class Connection : public event::EventSource {
    friend class impl::ConnectionProtocolAccess;

public:
    // defaults
    ~Connection() override = default;

public:
    /// Get the resolved local endpoint.
    [[nodiscard]] virtual auto localEndpoint() const -> std::optional<IpEndpoint> = 0;
    /// Get the resolved remote endpoint.
    [[nodiscard]] virtual auto remoteEndpoint() const -> std::optional<IpEndpoint> = 0;
    /// Get the configured application-stream buffer limits.
    [[nodiscard]] virtual auto bufferLimits() const noexcept -> SocketBufferLimits = 0;
    /// Get the connection lifecycle state.
    [[nodiscard]] virtual auto state() const noexcept -> ConnectionState = 0;
    /// Atomically submit one complete application byte block.
    /// @param data The owned stream data to queue.
    /// @return Whether the block was accepted, back-pressured, or rejected because the stream is closed.
    [[nodiscard]] virtual auto send(const mem::ByteBlock &data) -> NetworkSendStatus = 0;
    /// Suspend application-data delivery.
    virtual void pauseReceiving() = 0;
    /// Resume application-data delivery.
    virtual void resumeReceiving() = 0;
    /// Start protocol-specific graceful closure after accepted output drains.
    virtual void close() = 0;
    /// Abort the connection immediately.
    virtual void abort() noexcept = 0;
    /// Access the stable source-owned connection event editor.
    [[nodiscard]] auto events() -> ConnectionEventEditor & override = 0;

protected:
    /// Create a connection owned by an event collection.
    /// @param ownerEvents The owner-loop event collection.
    explicit Connection(event::EventsPtr ownerEvents) : EventSource{std::move(ownerEvents)} {}

private:
    std::shared_ptr<void> _exclusiveProtocolOwner; ///< Active internal protocol engine, if any.
    mem::ByteBlock _retainedProtocolInput;         ///< Bounded bytes handed to the next sequential engine.
};

}
