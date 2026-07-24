// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../unit/ByteLength.hpp"

namespace erbsland::network {

/// Finite send and receive buffer limits for network sources.
/// @tested{NetworkFacadeTest}
class SocketBufferLimits final {
public:
    /// Create limits of one MiB in each direction.
    constexpr SocketBufferLimits() noexcept = default;
    /// Create explicit send and receive queue limits.
    /// @param send The maximum queued output size.
    /// @param receive The maximum buffered input size.
    constexpr SocketBufferLimits(unit::ByteLength send, unit::ByteLength receive) noexcept :
        _send{send}, _receive{receive} {}

public: // accessors
    /// Get the send queue limit.
    /// @return The maximum queued output size.
    [[nodiscard]] constexpr auto send() const noexcept -> unit::ByteLength { return _send; }
    /// Get the receive buffer limit.
    /// @return The maximum buffered input size.
    [[nodiscard]] constexpr auto receive() const noexcept -> unit::ByteLength { return _receive; }

private:
    unit::ByteLength _send{1024U * 1024U};    ///< The maximum queued output size.
    unit::ByteLength _receive{1024U * 1024U}; ///< The maximum buffered input size.
};

}
