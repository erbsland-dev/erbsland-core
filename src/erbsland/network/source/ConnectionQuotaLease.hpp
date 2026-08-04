// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConnectionQuota_fwd.hpp"
#include "ConnectionQuotaLease_fwd.hpp"

#include "../IpEndpoint.hpp"

#include <optional>

namespace erbsland::network {

/// One move-only reservation from a shared connection quota.
/// The reservation is released exactly once, either explicitly or when this object is destroyed.
/// @tested{ConnectionQuotaTest TcpListenerTest TcpConnectionTest}
class ConnectionQuotaLease final {
    friend class ConnectionQuota;

public: // defaults/deletions
    /// Create an empty lease.
    ConnectionQuotaLease() = default;
    /// Release an acquired reservation.
    ~ConnectionQuotaLease();
    /// Move an acquired reservation.
    ConnectionQuotaLease(ConnectionQuotaLease &&other) noexcept;
    /// Replace this lease with another reservation.
    auto operator=(ConnectionQuotaLease &&other) noexcept -> ConnectionQuotaLease &;

    // defaults/deletions
    ConnectionQuotaLease(const ConnectionQuotaLease &) = delete;
    auto operator=(const ConnectionQuotaLease &) -> ConnectionQuotaLease & = delete;

public:
    /// Test whether this object owns a quota reservation.
    [[nodiscard]] auto isAcquired() const noexcept -> bool { return _quota != nullptr; }
    /// Release the reservation immediately.
    void release() noexcept;

private:
    /// Create an acquired reservation.
    ConnectionQuotaLease(ConnectionQuotaPtr quota, std::optional<IpEndpoint> remoteEndpoint) noexcept;

private:
    ConnectionQuotaPtr _quota;                 ///< Quota retaining this reservation.
    std::optional<IpEndpoint> _remoteEndpoint; ///< Endpoint retained for future keyed policies.
};

}
