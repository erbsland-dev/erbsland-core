// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UdpDatagramDropContext_fwd.hpp"
#include "UdpDatagramDropReason.hpp"

#include "../IpEndpoint.hpp"

#include "../../unit/ByteLength.hpp"

#include <optional>

namespace erbsland::network {

/// Details about one incoming UDP datagram discarded by the socket.
/// @tested{UdpSocketTest UdpSocketLiveTest}
class UdpDatagramDropContext final {
public:
    /// Create a datagram drop context.
    /// @param reason The reason for discarding the datagram.
    /// @param maximumSize The configured maximum payload size.
    /// @param remoteEndpoint The sender endpoint, if the platform reported it.
    /// @param datagramSize The original payload size, if the platform reported it.
    UdpDatagramDropContext(
        UdpDatagramDropReason reason,
        unit::ByteLength maximumSize,
        std::optional<IpEndpoint> remoteEndpoint = {},
        std::optional<unit::ByteLength> datagramSize = {}) noexcept :
        _remoteEndpoint{std::move(remoteEndpoint)},
        _datagramSize{datagramSize},
        _maximumSize{maximumSize},
        _reason{reason} {}

public: // accessors
    /// Get the reason for discarding the datagram.
    [[nodiscard]] constexpr auto reason() const noexcept -> UdpDatagramDropReason { return _reason; }
    /// Get the configured maximum payload size.
    [[nodiscard]] constexpr auto maximumSize() const noexcept -> unit::ByteLength { return _maximumSize; }
    /// Get the sender endpoint, if the platform reported it.
    [[nodiscard]] auto remoteEndpoint() const noexcept -> const std::optional<IpEndpoint> & { return _remoteEndpoint; }
    /// Get the original payload size, if the platform reported it.
    [[nodiscard]] auto datagramSize() const noexcept -> const std::optional<unit::ByteLength> & {
        return _datagramSize;
    }

private:
    std::optional<IpEndpoint> _remoteEndpoint;     ///< Optional sender endpoint.
    std::optional<unit::ByteLength> _datagramSize; ///< Optional original payload size.
    unit::ByteLength _maximumSize;                 ///< Configured maximum payload size.
    UdpDatagramDropReason _reason;                 ///< Reason for discarding the datagram.
};

}
