// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpAcceptOptions_fwd.hpp"

#include "../source/SocketBufferLimits.hpp"

namespace erbsland::network {

/// Options captured when a pending TCP connection is accepted.
/// @tested{TcpConnectionTest TcpListenerTest}
class TcpAcceptOptions final {
public: // accessors
    /// Get the connected stream buffer limits.
    [[nodiscard]] constexpr auto bufferLimits() const noexcept -> SocketBufferLimits { return _bufferLimits; }
    /// Set the connected stream buffer limits.
    auto setBufferLimits(const SocketBufferLimits value) noexcept -> TcpAcceptOptions & {
        _bufferLimits = value;
        return *this;
    }

private:
    SocketBufferLimits _bufferLimits; ///< Connected stream buffer limits.
};

}
