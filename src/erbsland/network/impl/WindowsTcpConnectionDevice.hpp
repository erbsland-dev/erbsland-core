// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifndef _WIN32
#error "WindowsTcpConnectionDevice.hpp is only available on Windows."
#endif

#include "TcpConnectionDevice.hpp"
#include "WindowsTcpConnectionState.hpp"

#include <memory>

namespace erbsland::network::impl {

/// Windows IOCP TCP connection device.
/// @tested{TcpSocketLiveTest}
class WindowsTcpConnectionDevice final : public TcpConnectionDevice {
public:
    /// Create a Windows TCP connection device.
    WindowsTcpConnectionDevice(
        event::EventLoopDriverPtr driver, unit::ByteLength receiveChunkLimit, TcpConnectionDeviceCallbacks callbacks);
    ~WindowsTcpConnectionDevice() override;

public: // implement TcpConnectionDevice
    void connect(IpEndpoint remoteEndpoint) override;
    void accept(TcpAcceptedSocketPtr socket) override;
    [[nodiscard]] auto canAccept(const TcpAcceptedSocket &socket) const noexcept -> bool override;
    [[nodiscard]] auto send(mem::ByteBlock data) -> TcpConnectionDeviceSendStatus override;
    void setReceiving(unit::ByteLength maximumBytes) override;
    void close() noexcept override;
    void abort() noexcept override;

private:
    std::shared_ptr<WindowsTcpConnectionState> _state; ///< Completion-owned state.
};

}
