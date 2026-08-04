// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpAcceptedSocket_fwd.hpp"
#include "TcpConnectionDevice_fwd.hpp"
#include "TcpConnectionDeviceCallbacks.hpp"
#include "TcpConnectionDeviceSendStatus.hpp"

#include "../IpEndpoint.hpp"

#include "../../event/EventLoopDriver_fwd.hpp"
#include "../../mem/ByteBlock.hpp"
#include "../../unit/ByteLength.hpp"

namespace erbsland::network::impl {

/// Platform-neutral interface for one connecting or established TCP socket.
/// @tested{TcpConnectionTest TcpSocketLiveTest}
class TcpConnectionDevice {
public:
    // defaults
    virtual ~TcpConnectionDevice() = default;

public: // factory
    /// Create the default native TCP connection device.
    [[nodiscard]] static auto create(
        event::EventLoopDriverPtr driver, unit::ByteLength receiveChunkLimit, TcpConnectionDeviceCallbacks callbacks)
        -> TcpConnectionDevicePtr;

public:
    /// Start a non-blocking connection to a numeric endpoint.
    virtual void connect(IpEndpoint remoteEndpoint) = 0;
    /// Adopt a transferable accepted socket.
    virtual void accept(TcpAcceptedSocketPtr socket) = 0;
    /// Test whether this device can adopt a given accepted socket.
    [[nodiscard]] virtual auto canAccept(const TcpAcceptedSocket &socket) const noexcept -> bool = 0;
    /// Submit one complete block, retaining it when native completion is pending.
    [[nodiscard]] virtual auto send(mem::ByteBlock data) -> TcpConnectionDeviceSendStatus = 0;
    /// Set the largest next native receive, or zero to suspend receiving.
    virtual void setReceiving(unit::ByteLength maximumBytes) = 0;
    /// Close the native socket.
    virtual void close() noexcept = 0;
    /// Abort the native socket immediately.
    virtual void abort() noexcept = 0;
};

}
