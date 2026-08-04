// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsServerConnectionCloseContext_fwd.hpp"
#include "TlsServerConnectionCloseOrigin.hpp"

namespace erbsland::network {

/// Details of one orderly accepted TLS closure.
/// @tested{TlsServerConnectionTest}
class TlsServerConnectionCloseContext final {
public:
    /// Create closure details.
    /// @param origin The side that first sent or requested close-notify.
    explicit constexpr TlsServerConnectionCloseContext(const TlsServerConnectionCloseOrigin origin) noexcept :
        _origin{origin} {}
    /// Get the side that initiated orderly TLS closure.
    [[nodiscard]] constexpr auto origin() const noexcept -> TlsServerConnectionCloseOrigin { return _origin; }

private:
    TlsServerConnectionCloseOrigin _origin; ///< First orderly-close initiator.
};

}
