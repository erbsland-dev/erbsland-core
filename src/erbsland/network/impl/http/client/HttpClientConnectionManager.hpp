// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../../event/Events_fwd.hpp"
#include "../../../../time/TimePoint.hpp"
#include "../../../http_client/HttpClientSessionOptions.hpp"
#include "../../../source/Connection_fwd.hpp"
#include "../../../url/Url.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace erbsland::network::impl {

/// Session-owned sequential HTTP connection reuse manager.
/// @tested{HttpClientTest}
class HttpClientConnectionManager final : public std::enable_shared_from_this<HttpClientConnectionManager> {
public:
    /// One reusable connection leased to a request.
    struct Lease {
        ConnectionPtr connection;              ///< Leased active connection.
        unit::ItemCount completedTransactions; ///< Transactions completed before this lease.
    };

private:
    /// One connection retained while idle.
    struct IdleConnection {
        text::String origin;                   ///< Canonical HTTP origin.
        ConnectionPtr connection;              ///< Retained active connection.
        unit::ItemCount completedTransactions; ///< Completed transaction count.
        time::TimePoint deadline;              ///< Idle expiry deadline.
        std::uint64_t generation{};            ///< Captured transport configuration generation.
        std::uint64_t identity{};              ///< Stable callback identity.
        std::uint64_t sequence{};              ///< Retention sequence for global eviction.
    };

public:
    /// Create a manager owned by one event collection.
    explicit HttpClientConnectionManager(event::EventsPtr ownerEvents);
    /// Close all retained idle connections.
    ~HttpClientConnectionManager();

public:
    /// Lease one eligible idle connection for an origin and configuration generation.
    [[nodiscard]] auto lease(const Url &url, std::uint64_t generation) -> std::optional<Lease>;
    /// Return a completed reusable connection under captured session limits.
    void returnConnection(
        const Url &url,
        std::uint64_t generation,
        ConnectionPtr connection,
        unit::ItemCount completedTransactions,
        const HttpClientSessionOptions &options);
    /// Close and forget every idle connection.
    void invalidate() noexcept;

private:
    /// Build the canonical HTTP origin key for a URL.
    [[nodiscard]] static auto originKey(const Url &url) -> text::String;
    /// Evict an idle connection if its callback identity still matches.
    void evict(const text::String &origin, std::uint64_t identity) noexcept;
    /// Evict the globally oldest idle connection.
    void evictOldest() noexcept;

private:
    event::EventsPtr _ownerEvents;
    std::vector<IdleConnection> _idle;
    std::uint64_t _identity{};
    std::uint64_t _sequence{};
};

using HttpClientConnectionManagerPtr = std::shared_ptr<HttpClientConnectionManager>;

}
