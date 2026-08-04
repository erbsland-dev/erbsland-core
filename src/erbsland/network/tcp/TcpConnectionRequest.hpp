// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpConnectionRequest_fwd.hpp"
#include "TcpConnectionRequestState.hpp"

#include "../IpEndpoint.hpp"

namespace erbsland::network {

/// A pending incoming TCP connection decision.
/// A request does not emit callbacks and is therefore not an `EventSource`. Listener implementations enforce its
/// owner-loop affinity while this object only represents the accept-or-reject decision.
/// Concrete implementations reject an undecided request when it is destroyed.
/// @notest{Abstract interface; listener implementations own request behavior tests.}
class TcpConnectionRequest {
public:
    // defaults
    virtual ~TcpConnectionRequest() = default;

public:
    /// Get the resolved remote endpoint.
    /// @return The connecting peer endpoint.
    [[nodiscard]] virtual auto remoteEndpoint() const noexcept -> const IpEndpoint & = 0;
    /// Get the request lifecycle state.
    /// @return The current state.
    [[nodiscard]] virtual auto state() const noexcept -> TcpConnectionRequestState = 0;
    /// Reject the pending request immediately.
    /// This method is thread-safe and idempotent.
    virtual void reject() noexcept = 0;
};

}
