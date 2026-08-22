// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifndef _WIN32
#error "WindowsTcpListenerDevice.hpp is only available on Windows."
#endif

#include "WindowsTcpListenerState.hpp"

#include "../TcpListenerDevice.hpp"

#include <memory>

namespace erbsland::network::impl {

/// Windows IOCP TCP listener device.
/// @tested{TcpSocketLiveTest}
class WindowsTcpListenerDevice final : public TcpListenerDevice {
public:
    /// Create a Windows TCP listener device.
    WindowsTcpListenerDevice(event::EventLoopDriverPtr driver, TcpListenerDeviceCallbacks callbacks);
    ~WindowsTcpListenerDevice() override;

public: // implement TcpListenerDevice
    [[nodiscard]] auto start(IpEndpoint localEndpoint, unit::ItemCount backlog) -> IpEndpoint override;
    void setAccepting(bool enabled) override;
    void close() noexcept override;
    void abort() noexcept override;

private:
    std::shared_ptr<WindowsTcpListenerState> _state; ///< Completion-owned state.
};

}
