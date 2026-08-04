// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#if !defined(__APPLE__) && !defined(__linux__)
#error "PosixTcpSocket.hpp is only available on macOS and Linux."
#endif

#include "TcpAcceptedSocket.hpp"

namespace erbsland::network::impl {

/// Transferable POSIX TCP socket accepted by a listener.
/// @tested{TcpSocketLiveTest}
class PosixTcpSocket final : public TcpAcceptedSocket {
public:
    /// Adopt an accepted POSIX TCP socket.
    PosixTcpSocket(int descriptor, IpEndpoint localEndpoint, IpEndpoint remoteEndpoint) noexcept;
    ~PosixTcpSocket() override;

    // defaults/deletions
    PosixTcpSocket(const PosixTcpSocket &) = delete;
    auto operator=(const PosixTcpSocket &) -> PosixTcpSocket & = delete;

public: // implement TcpAcceptedSocket
    [[nodiscard]] auto localEndpoint() const noexcept -> const IpEndpoint & override;
    [[nodiscard]] auto remoteEndpoint() const noexcept -> const IpEndpoint & override;

public:
    /// Transfer ownership of the native descriptor.
    [[nodiscard]] auto takeDescriptor() noexcept -> int;

private:
    int _descriptor{-1};        ///< Owned native socket descriptor.
    IpEndpoint _localEndpoint;  ///< Local endpoint.
    IpEndpoint _remoteEndpoint; ///< Remote endpoint.
};

}
