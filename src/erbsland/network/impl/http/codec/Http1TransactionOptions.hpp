// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Http1CodecLimits.hpp"

#include "../../../../time/TimeDelta.hpp"

namespace erbsland::network::impl {

/// Captured limits and deadlines for one internal HTTP/1 transaction.
/// @tested{Http1TransactionTest}
class Http1TransactionOptions final {
public:
    inline static const auto cDefaultHeaderTimeout = time::TimeDelta::seconds(30);
    inline static const auto cDefaultBodyIdleTimeout = time::TimeDelta::minutes(5);
    inline static const auto cDefaultTotalTimeout = time::TimeDelta::minutes(30);
    inline static const auto cDefaultCloseTimeout = time::TimeDelta::seconds(10);

public:
    /// Get the HTTP codec limits.
    [[nodiscard]] auto codecLimits() const noexcept -> Http1CodecLimits { return _codecLimits; }
    /// Set the HTTP codec limits.
    auto setCodecLimits(Http1CodecLimits value) noexcept -> Http1TransactionOptions & {
        _codecLimits = value;
        return *this;
    }
    /// Get the incoming-head deadline.
    [[nodiscard]] auto headerTimeout() const noexcept -> time::TimeDelta { return _headerTimeout; }
    /// Set the incoming-head deadline.
    auto setHeaderTimeout(time::TimeDelta value) noexcept -> Http1TransactionOptions & {
        _headerTimeout = value;
        return *this;
    }
    /// Get the decoded-body idle deadline.
    [[nodiscard]] auto bodyIdleTimeout() const noexcept -> time::TimeDelta { return _bodyIdleTimeout; }
    /// Set the decoded-body idle deadline.
    auto setBodyIdleTimeout(time::TimeDelta value) noexcept -> Http1TransactionOptions & {
        _bodyIdleTimeout = value;
        return *this;
    }
    /// Get the absolute transaction deadline.
    [[nodiscard]] auto totalTimeout() const noexcept -> time::TimeDelta { return _totalTimeout; }
    /// Set the absolute transaction deadline.
    auto setTotalTimeout(time::TimeDelta value) noexcept -> Http1TransactionOptions & {
        _totalTimeout = value;
        return *this;
    }
    /// Get the graceful-close deadline.
    [[nodiscard]] auto closeTimeout() const noexcept -> time::TimeDelta { return _closeTimeout; }
    /// Set the graceful-close deadline.
    auto setCloseTimeout(time::TimeDelta value) noexcept -> Http1TransactionOptions & {
        _closeTimeout = value;
        return *this;
    }

private:
    Http1CodecLimits _codecLimits;                             ///< HTTP parsing and serialization limits.
    time::TimeDelta _headerTimeout{cDefaultHeaderTimeout};     ///< Incoming-head deadline.
    time::TimeDelta _bodyIdleTimeout{cDefaultBodyIdleTimeout}; ///< Decoded-body idle deadline.
    time::TimeDelta _totalTimeout{cDefaultTotalTimeout};       ///< Absolute transaction deadline.
    time::TimeDelta _closeTimeout{cDefaultCloseTimeout};       ///< Graceful connection-close deadline.
};

}
