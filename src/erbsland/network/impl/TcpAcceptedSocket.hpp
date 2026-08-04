// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpAcceptedSocket_fwd.hpp"

#include "../IpEndpoint.hpp"

namespace erbsland::network::impl {

/// A transferable native socket accepted by a TCP listener.
/// @tested{TcpListenerTest TcpSocketLiveTest}
class TcpAcceptedSocket {
public:
    // defaults
    virtual ~TcpAcceptedSocket() = default;

public:
    /// Get the local endpoint.
    [[nodiscard]] virtual auto localEndpoint() const noexcept -> const IpEndpoint & = 0;
    /// Get the remote endpoint.
    [[nodiscard]] virtual auto remoteEndpoint() const noexcept -> const IpEndpoint & = 0;
};

}
