// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpConnectionCloseContext_fwd.hpp"
#include "TcpConnectionCloseOrigin.hpp"

namespace erbsland::network {

/// Context for a normal TCP connection closure.
/// @tested{TcpConnectionTest TcpSocketLiveTest}
class TcpConnectionCloseContext final {
public:
    /// Create a close context.
    /// @param origin The side that initiated closure first.
    explicit constexpr TcpConnectionCloseContext(const TcpConnectionCloseOrigin origin) noexcept : _origin{origin} {}

public: // accessors
    /// Get the side that initiated closure first.
    [[nodiscard]] constexpr auto origin() const noexcept -> TcpConnectionCloseOrigin { return _origin; }

private:
    TcpConnectionCloseOrigin _origin; ///< Side that initiated closure first.
};

}
