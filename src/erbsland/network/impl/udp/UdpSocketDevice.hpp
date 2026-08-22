// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UdpSocketDevice_fwd.hpp"
#include "UdpSocketDeviceCallbacks.hpp"
#include "UdpSocketDeviceSendStatus.hpp"

#include "../../../event/EventLoopDriver_fwd.hpp"
#include "../../udp/UdpDatagram.hpp"

namespace erbsland::network::impl {

/// Platform-neutral interface for one bound native UDP socket.
/// @tested{UdpSocketTest UdpSocketLiveTest}
class UdpSocketDevice {
public:
    // defaults
    virtual ~UdpSocketDevice() = default;

public:
    /// Create the default native UDP device for the current platform.
    /// @param driver The native event-loop driver.
    /// @param callbacks The source callbacks.
    /// @return The platform-native UDP device.
    /// @throws err::RuntimeError If the driver is incompatible with the platform implementation.
    [[nodiscard]] static auto createUdpSocketDevice(
        event::EventLoopDriverPtr driver, UdpSocketDeviceCallbacks callbacks) -> UdpSocketDevicePtr;
    /// Bind the native socket.
    /// @param localEndpoint The requested local endpoint.
    /// @param maximumDatagramSize The receive buffer and payload limit.
    /// @return The actual bound endpoint.
    /// @throws NetworkError If native socket creation or binding fails.
    [[nodiscard]] virtual auto bind(IpEndpoint localEndpoint, unit::ByteLength maximumDatagramSize) -> IpEndpoint = 0;
    /// Submit one datagram to the native device.
    /// @param datagram The datagram retained by the owning source until completion.
    /// @return Whether the native operation completed, is pending, or would block.
    /// @throws NetworkError If the native submission fails.
    [[nodiscard]] virtual auto send(const UdpDatagram &datagram) -> UdpSocketDeviceSendStatus = 0;
    /// Enable or disable native receive operations.
    /// @param enabled Whether receive operations shall be active.
    virtual void setReceiving(bool enabled) = 0;
    /// Close and unregister the device on its owner event loop.
    virtual void close() noexcept = 0;
    /// Close the native socket immediately from any thread.
    virtual void abort() noexcept = 0;
};

}
