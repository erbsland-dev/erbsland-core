// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixTcpSocket.hpp"

#include <unistd.h>

namespace erbsland::network::impl {

PosixTcpSocket::PosixTcpSocket(const int descriptor, IpEndpoint localEndpoint, IpEndpoint remoteEndpoint) noexcept :
    _descriptor{descriptor}, _localEndpoint{std::move(localEndpoint)}, _remoteEndpoint{std::move(remoteEndpoint)} {
}

PosixTcpSocket::~PosixTcpSocket() {
    if (_descriptor >= 0) {
        // A close failure cannot be reported from a destructor or retried safely.
        ::close(_descriptor);
    }
}

auto PosixTcpSocket::localEndpoint() const noexcept -> const IpEndpoint & {
    return _localEndpoint;
}

auto PosixTcpSocket::remoteEndpoint() const noexcept -> const IpEndpoint & {
    return _remoteEndpoint;
}

auto PosixTcpSocket::takeDescriptor() noexcept -> int {
    const auto result = _descriptor;
    _descriptor = -1;
    return result;
}

}
