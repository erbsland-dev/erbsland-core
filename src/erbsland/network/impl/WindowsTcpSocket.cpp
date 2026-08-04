// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsTcpSocket.hpp"

namespace erbsland::network::impl {

WindowsTcpSocket::WindowsTcpSocket(
    std::shared_ptr<WindowsNetworkRuntime> runtime,
    const SOCKET socket,
    IpEndpoint localEndpoint,
    IpEndpoint remoteEndpoint) noexcept :
    _runtime{std::move(runtime)},
    _socket{socket},
    _localEndpoint{std::move(localEndpoint)},
    _remoteEndpoint{std::move(remoteEndpoint)} {
}

WindowsTcpSocket::~WindowsTcpSocket() {
    if (_socket != INVALID_SOCKET) {
        // A destructor cannot report failure and the socket has no owner left to recover it.
        ::closesocket(_socket);
    }
}

auto WindowsTcpSocket::localEndpoint() const noexcept -> const IpEndpoint & {
    return _localEndpoint;
}

auto WindowsTcpSocket::remoteEndpoint() const noexcept -> const IpEndpoint & {
    return _remoteEndpoint;
}

auto WindowsTcpSocket::takeSocket() noexcept -> SOCKET {
    const auto result = _socket;
    _socket = INVALID_SOCKET;
    return result;
}

}
