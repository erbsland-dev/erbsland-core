// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConnectionQuota_fwd.hpp"
#include "ConnectionQuotaLease.hpp"

#include "../impl/tcp/TcpListener_fwd.hpp"
#include "../tcp/TcpListener_fwd.hpp"

#include "../../unit/ItemCount.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>

namespace erbsland::network {

/// A thread-safe shared limit for concurrent connection-related work.
/// A quota can be shared by listeners on different event loops. Acquisition retains optional remote-endpoint context
/// so future keyed policies can be added without changing connection APIs.
/// @tested{ConnectionQuotaTest TcpListenerTest TcpConnectionTest}
class ConnectionQuota final : public std::enable_shared_from_this<ConnectionQuota> {
    friend class ConnectionQuotaLease;
    friend class impl::TcpListener;

private:
    using CapacityCallback = std::function<void()>;

public:
    /// Create a quota with a positive finite maximum.
    /// @param maximum The maximum number of simultaneous leases.
    /// @return The new shared quota.
    /// @throws err::ParameterError If `maximum` is zero or infinite.
    [[nodiscard]] static auto create(unit::ItemCount maximum) -> ConnectionQuotaPtr;

public: // defaults/deletions
    ~ConnectionQuota() = default;
    ConnectionQuota(const ConnectionQuota &) = delete;
    ConnectionQuota(ConnectionQuota &&) = delete;
    auto operator=(const ConnectionQuota &) -> ConnectionQuota & = delete;
    auto operator=(ConnectionQuota &&) -> ConnectionQuota & = delete;

public:
    /// Try to reserve one slot without blocking.
    /// @param remoteEndpoint Optional remote endpoint associated with the work.
    /// @return An acquired lease, or no value when the quota is full.
    [[nodiscard]] auto tryAcquire(std::optional<IpEndpoint> remoteEndpoint = {}) -> std::optional<ConnectionQuotaLease>;
    /// Get the configured maximum.
    [[nodiscard]] auto maximum() const noexcept -> unit::ItemCount { return _maximum; }
    /// Get the current number of acquired leases.
    [[nodiscard]] auto current() const noexcept -> unit::ItemCount;
    /// Get the currently available capacity.
    [[nodiscard]] auto available() const noexcept -> unit::ItemCount;

private:
    /// Create a validated quota.
    explicit ConnectionQuota(unit::ItemCount maximum) noexcept;
    /// Release one slot and notify capacity subscribers.
    void release() noexcept;
    /// Register a capacity-change callback.
    [[nodiscard]] auto subscribe(CapacityCallback callback) -> std::uint64_t;
    /// Remove a capacity-change callback.
    void unsubscribe(std::uint64_t id) noexcept;

private:
    unit::ItemCount _maximum;                                         ///< Maximum simultaneous leases.
    mutable std::mutex _mutex;                                        ///< Protects counts and subscribers.
    std::size_t _current{};                                           ///< Current acquired lease count.
    std::uint64_t _nextSubscriptionId{1U};                            ///< Next nonzero subscriber identifier.
    std::unordered_map<std::uint64_t, CapacityCallback> _subscribers; ///< Capacity-change callbacks.
};

}
