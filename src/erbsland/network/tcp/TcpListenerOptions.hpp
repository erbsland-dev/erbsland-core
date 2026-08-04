// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpConnectionFilterFn.hpp"
#include "TcpListener_fwd.hpp"
#include "TcpListenerOptions_fwd.hpp"

#include "../impl/TcpListener_fwd.hpp"
#include "../source/ConnectionQuota.hpp"

#include "../../unit/ItemCount.hpp"

#include <utility>

namespace erbsland::network {

/// Options captured when a TCP listener is started.
/// The connection filter runs synchronously on the listener event loop and must remain fast and non-blocking.
/// @tested{TcpListenerTest}
class TcpListenerOptions final {
    friend class impl::TcpListener;

public:
    /// The default native pending-connection backlog.
    inline static constexpr auto cDefaultBacklog = unit::ItemCount{128U};
    /// The default maximum number of emitted requests awaiting a decision.
    inline static constexpr auto cDefaultMaximumPendingRequests = unit::ItemCount{128U};
    /// The default maximum number of accepted sockets retained by one listener quota.
    inline static constexpr auto cDefaultMaximumConnections = unit::ItemCount{1024U};

public:
    /// Create options with one private bounded connection quota.
    TcpListenerOptions();

public: // accessors
    /// Get the requested native pending-connection backlog.
    [[nodiscard]] constexpr auto backlog() const noexcept -> unit::ItemCount { return _backlog; }
    /// Set the requested native pending-connection backlog.
    auto setBacklog(const unit::ItemCount value) noexcept -> TcpListenerOptions & {
        _backlog = value;
        return *this;
    }
    /// Get the maximum number of emitted requests awaiting a decision.
    [[nodiscard]] constexpr auto maximumPendingRequests() const noexcept -> unit::ItemCount {
        return _maximumPendingRequests;
    }
    /// Set the maximum number of emitted requests awaiting a decision.
    auto setMaximumPendingRequests(const unit::ItemCount value) noexcept -> TcpListenerOptions & {
        _maximumPendingRequests = value;
        return *this;
    }
    /// Get the synchronous incoming-connection filter.
    [[nodiscard]] auto connectionFilter() const noexcept -> const TcpConnectionFilterFn & { return _connectionFilter; }
    /// Set the synchronous incoming-connection filter.
    auto setConnectionFilter(TcpConnectionFilterFn value) -> TcpListenerOptions & {
        _connectionFilter = std::move(value);
        return *this;
    }
    /// Get the shared accepted-connection quota.
    [[nodiscard]] auto connectionQuota() const noexcept -> const ConnectionQuotaPtr & { return _connectionQuota; }
    /// Set the quota shared by accepted connections and optionally multiple listeners.
    /// @param value The non-null shared quota.
    /// @return This options object for chaining.
    auto setConnectionQuota(ConnectionQuotaPtr value) noexcept -> TcpListenerOptions & {
        _connectionQuota = std::move(value);
        _hasExplicitConnectionQuota = true;
        return *this;
    }

private:
    unit::ItemCount _backlog{cDefaultBacklog};                               ///< Native listen backlog.
    unit::ItemCount _maximumPendingRequests{cDefaultMaximumPendingRequests}; ///< Emitted request limit.
    TcpConnectionFilterFn _connectionFilter;                                 ///< Synchronous admission filter.
    ConnectionQuotaPtr _connectionQuota;                                     ///< Accepted socket lifetime quota.
    bool _hasExplicitConnectionQuota{};                                      ///< Whether the caller shared a quota.
};

}
