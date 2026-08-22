// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifndef _WIN32
#error "WindowsTcpSocket.hpp is only available on Windows."
#endif

#include "../TcpAcceptedSocket.hpp"

#include "../../../../core/impl/WindowsApi.hpp"
#include "../../platform/WindowsNetworkRuntime.hpp"

#include <winsock2.h>

#include <memory>

namespace erbsland::network::impl {

/// Transferable Windows TCP socket accepted by a listener.
/// @tested{TcpSocketLiveTest}
class WindowsTcpSocket final : public TcpAcceptedSocket {
public:
    /// Adopt an accepted Windows TCP socket.
    WindowsTcpSocket(
        std::shared_ptr<WindowsNetworkRuntime> runtime,
        SOCKET socket,
        IpEndpoint localEndpoint,
        IpEndpoint remoteEndpoint) noexcept;
    ~WindowsTcpSocket() override;

    // defaults/deletions
    WindowsTcpSocket(const WindowsTcpSocket &) = delete;
    auto operator=(const WindowsTcpSocket &) -> WindowsTcpSocket & = delete;

public: // implement TcpAcceptedSocket
    [[nodiscard]] auto localEndpoint() const noexcept -> const IpEndpoint & override;
    [[nodiscard]] auto remoteEndpoint() const noexcept -> const IpEndpoint & override;

public:
    /// Transfer ownership of the native socket.
    [[nodiscard]] auto takeSocket() noexcept -> SOCKET;

private:
    std::shared_ptr<WindowsNetworkRuntime> _runtime; ///< Shared Winsock lifetime.
    SOCKET _socket{INVALID_SOCKET};                  ///< Owned native socket.
    IpEndpoint _localEndpoint;                       ///< Local endpoint.
    IpEndpoint _remoteEndpoint;                      ///< Remote endpoint.
};

}
