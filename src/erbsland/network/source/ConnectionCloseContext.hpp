// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConnectionCloseContext_fwd.hpp"
#include "ConnectionCloseOrigin.hpp"

namespace erbsland::network {

/// Context for one orderly byte-stream connection closure.
/// A remote TCP close represents peer EOF; a remote TLS close represents an authenticated `close_notify`.
/// @tested{ConnectionTest TcpConnectionTest TlsClientConnectionTest TlsServerConnectionTest}
class ConnectionCloseContext final {
public:
    /// Create a close context.
    /// @param origin The side that initiated orderly closure first.
    explicit constexpr ConnectionCloseContext(const ConnectionCloseOrigin origin) noexcept : _origin{origin} {}

public:
    /// Get the side that initiated orderly closure first.
    [[nodiscard]] constexpr auto origin() const noexcept -> ConnectionCloseOrigin { return _origin; }

private:
    ConnectionCloseOrigin _origin; ///< First orderly-close initiator.
};

}
