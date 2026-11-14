// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpAcceptedSocket_fwd.hpp"
#include "TcpConnectionDevice_fwd.hpp"
#include "TcpConnectionDeviceCallbacks.hpp"
#include "TcpConnectionDeviceSendStatus.hpp"

#include "../../../event/EventLoopDriver_fwd.hpp"
#include "../../../mem/ByteBlock.hpp"
#include "../../../unit/ByteCount.hpp"
#include "../../IpEndpoint.hpp"

namespace erbsland::network::impl {

/// Platform-neutral interface for one connecting or established TCP socket.
/// Callers retain shared ownership throughout each operation and explicitly close or abort before releasing the device.
/// @tested{TcpConnectionTest TcpSocketLiveTest}
class TcpConnectionDevice {
public:
    // defaults
    virtual ~TcpConnectionDevice() = default;

public: // factory
    /// Create the default native TCP connection device.
    /// @param driver The platform's default native event-loop driver; custom drivers are not supported.
    /// @param receiveChunkLimit The maximum native receive size.
    /// @param callbacks The source callbacks.
    /// @return The platform-native connection device.
    [[nodiscard]] static auto create(
        event::EventLoopDriverPtr driver, unit::ByteCount receiveChunkLimit, TcpConnectionDeviceCallbacks callbacks)
        -> TcpConnectionDevicePtr;

public:
    /// Start a non-blocking connection to a numeric endpoint.
    /// @param remoteEndpoint The remote numeric endpoint.
    /// @throws NetworkError If socket creation, configuration, or connection fails.
    /// @throws system::PlatformError If native event registration fails.
    virtual void connect(IpEndpoint remoteEndpoint) = 0;
    /// Adopt a transferable accepted socket.
    /// @param socket A non-null socket for which `canAccept()` returns true.
    /// @throws NetworkError If socket configuration fails.
    /// @throws system::PlatformError If native event registration fails.
    virtual void accept(TcpAcceptedSocketPtr socket) = 0;
    /// Test whether this device can adopt a given accepted socket.
    [[nodiscard]] virtual auto canAccept(const TcpAcceptedSocket &socket) const noexcept -> bool = 0;
    /// Submit one complete block, retaining it when native completion is pending.
    /// No previous output block may remain pending.
    /// @param data The complete output block.
    /// @return The immediate native submission status.
    /// @throws NetworkError If native submission fails.
    /// @throws system::PlatformError If native event registration fails.
    [[nodiscard]] virtual auto send(mem::ByteBlock data) -> TcpConnectionDeviceSendStatus = 0;
    /// Set the largest next native receive, or zero to suspend receiving.
    /// @param maximumBytes The receive size limit, or zero.
    /// @throws system::PlatformError If native event registration fails.
    virtual void setReceiving(unit::ByteCount maximumBytes) = 0;
    /// Close the native socket.
    virtual void close() noexcept = 0;
    /// Abort the native socket immediately.
    virtual void abort() noexcept = 0;
};

}
