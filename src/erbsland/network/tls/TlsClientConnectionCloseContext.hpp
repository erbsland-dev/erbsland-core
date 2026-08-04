// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsClientConnectionCloseContext_fwd.hpp"
#include "TlsClientConnectionCloseOrigin.hpp"

namespace erbsland::network {

/// Context for a completed bidirectional TLS closure.
/// @tested{TlsClientConnectionTest}
class TlsClientConnectionCloseContext final {
public:
    /// Create a close context.
    explicit constexpr TlsClientConnectionCloseContext(const TlsClientConnectionCloseOrigin origin) noexcept :
        _origin{origin} {}

public:
    /// Get the side that initiated TLS closure first.
    [[nodiscard]] constexpr auto origin() const noexcept -> TlsClientConnectionCloseOrigin { return _origin; }

private:
    TlsClientConnectionCloseOrigin _origin; ///< First orderly-close initiator.
};

}
