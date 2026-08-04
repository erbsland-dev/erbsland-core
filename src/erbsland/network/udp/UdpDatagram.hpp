// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../IpEndpoint.hpp"

#include "../../mem/ByteBlock.hpp"

namespace erbsland::network {

/// One owned UDP datagram and its remote endpoint.
/// @tested{NetworkFacadeTest}
class UdpDatagram final {
public:
    /// Create an empty datagram for the IPv4 any endpoint.
    UdpDatagram() = default;
    /// Create an owned datagram.
    /// @param remoteEndpoint The remote source or destination endpoint.
    /// @param data The owned datagram payload.
    UdpDatagram(IpEndpoint remoteEndpoint, mem::ByteBlock data) noexcept :
        _remoteEndpoint{std::move(remoteEndpoint)}, _data{std::move(data)} {}

public: // accessors
    /// Get the remote endpoint.
    /// @return The datagram source or destination endpoint.
    [[nodiscard]] auto remoteEndpoint() const noexcept -> const IpEndpoint & { return _remoteEndpoint; }
    /// Get the owned payload.
    /// @return The datagram bytes.
    [[nodiscard]] auto data() const noexcept -> const mem::ByteBlock & { return _data; }

private:
    IpEndpoint _remoteEndpoint; ///< The remote source or destination endpoint.
    mem::ByteBlock _data;       ///< The owned payload.
};

}
