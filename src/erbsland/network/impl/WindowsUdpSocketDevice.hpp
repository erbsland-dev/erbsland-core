// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UdpSocketDevice.hpp"
#include "WindowsUdpSocketState_fwd.hpp"

#include <memory>

namespace erbsland::network::impl {

/// Windows IOCP implementation of a native UDP socket device.
/// @tested{UdpSocketLiveTest}
class WindowsUdpSocketDevice final : public UdpSocketDevice {
public:
    /// Create a Windows UDP socket device.
    WindowsUdpSocketDevice(event::EventLoopDriverPtr driver, UdpSocketDeviceCallbacks callbacks);
    ~WindowsUdpSocketDevice() override;

public: // implement UdpSocketDevice
    [[nodiscard]] auto bind(IpEndpoint localEndpoint, unit::ByteLength maximumDatagramSize) -> IpEndpoint override;
    [[nodiscard]] auto send(const UdpDatagram &datagram) -> UdpSocketDeviceSendStatus override;
    void setReceiving(bool enabled) override;
    void close() noexcept override;
    void abort() noexcept override;

private:
    std::shared_ptr<WindowsUdpSocketState> _state; ///< Completion-owned device state.
};

}
