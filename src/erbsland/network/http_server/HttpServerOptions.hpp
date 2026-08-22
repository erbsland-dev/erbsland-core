// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../http/HttpHeaderLimits.hpp"

#include "../../time/TimeDelta.hpp"
#include "../../unit/ByteLength.hpp"
#include "../../unit/ItemCount.hpp"

namespace erbsland::network {

/// Limits and deadlines captured when an HTTP server starts.
/// @tested{HttpServerLiveTest}
class HttpServerOptions final {
public:
    /// Default deadline for a complete request head.
    inline static const auto cDefaultHeaderTimeout = time::TimeDelta::seconds(30);
    /// Default rolling deadline while receiving a body.
    inline static const auto cDefaultBodyIdleTimeout = time::TimeDelta::minutes(5);
    /// Default absolute transaction deadline.
    inline static const auto cDefaultTotalTimeout = time::TimeDelta::minutes(30);
    /// Default graceful connection-close deadline.
    inline static const auto cDefaultCloseTimeout = time::TimeDelta::seconds(10);
    /// Default maximum request-line length.
    static constexpr auto cDefaultMaximumStartLineLength = unit::ByteLength{8U * 1024U};
    /// Default decoded request-body maximum.
    static constexpr auto cDefaultMaximumBodyLength = unit::ByteLength{16U * 1024U * 1024U};
    /// Default transaction input and output queue maximum.
    static constexpr auto cDefaultMaximumQueueLength = unit::ByteLength{1024U * 1024U};
    /// Default fixed-response body maximum.
    static constexpr auto cDefaultMaximumFixedResponseLength = unit::ByteLength{256U * 1024U};
    /// Default finite keep-alive transaction count.
    static constexpr auto cDefaultMaximumRequestsPerConnection = unit::ItemCount{1000U};
    /// Default maximum number of active static-content responses and open files.
    static constexpr auto cDefaultMaximumStaticContentResponses = unit::ItemCount{128U};
    /// Default maximum number of queued blocking content operations.
    static constexpr auto cDefaultMaximumStaticContentOperations = unit::ItemCount{256U};
    /// Default maximum bytes retained between content workers and HTTP output.
    static constexpr auto cDefaultMaximumStaticContentQueueLength = unit::ByteLength{2U * 1024U * 1024U};
    /// Default maximum aggregate static-content memory retained by active responses.
    static constexpr auto cDefaultMaximumRetainedStaticContentMemoryLength = unit::ByteLength{16U * 1024U * 1024U};
    /// Default and hard maximum for one static-content output block.
    static constexpr auto cDefaultStaticContentChunkLength = unit::ByteLength{16U * 1024U};

public: // accessors
    /// Get the request and response header limits.
    [[nodiscard]] auto headerLimits() const noexcept -> HttpHeaderLimits { return _headerLimits; }
    /// Set the request and response header limits.
    auto setHeaderLimits(HttpHeaderLimits value) noexcept -> HttpServerOptions & {
        _headerLimits = value;
        return *this;
    }
    /// Get the maximum request-line length.
    [[nodiscard]] auto maximumStartLineLength() const noexcept -> unit::ByteLength { return _maximumStartLineLength; }
    /// Set the maximum request-line length.
    auto setMaximumStartLineLength(unit::ByteLength value) noexcept -> HttpServerOptions & {
        _maximumStartLineLength = value;
        return *this;
    }
    /// Get the decoded request-body hard maximum.
    [[nodiscard]] auto maximumBodyLength() const noexcept -> unit::ByteLength { return _maximumBodyLength; }
    /// Set the decoded request-body hard maximum.
    auto setMaximumBodyLength(unit::ByteLength value) noexcept -> HttpServerOptions & {
        _maximumBodyLength = value;
        return *this;
    }
    /// Get the transaction queue maximum.
    [[nodiscard]] auto maximumQueueLength() const noexcept -> unit::ByteLength { return _maximumQueueLength; }
    /// Set the transaction queue maximum.
    auto setMaximumQueueLength(unit::ByteLength value) noexcept -> HttpServerOptions & {
        _maximumQueueLength = value;
        return *this;
    }
    /// Get the request-bound fixed-response body maximum.
    [[nodiscard]] auto maximumFixedResponseLength() const noexcept -> unit::ByteLength {
        return _maximumFixedResponseLength;
    }
    /// Set the request-bound fixed-response body maximum.
    auto setMaximumFixedResponseLength(unit::ByteLength value) noexcept -> HttpServerOptions & {
        _maximumFixedResponseLength = value;
        return *this;
    }
    /// Get the finite number of transactions accepted per connection.
    [[nodiscard]] auto maximumRequestsPerConnection() const noexcept -> unit::ItemCount {
        return _maximumRequestsPerConnection;
    }
    /// Set the finite number of transactions accepted per connection.
    auto setMaximumRequestsPerConnection(unit::ItemCount value) noexcept -> HttpServerOptions & {
        _maximumRequestsPerConnection = value;
        return *this;
    }
    /// Get the maximum number of active static-content responses.
    [[nodiscard]] auto maximumStaticContentResponses() const noexcept -> unit::ItemCount {
        return _maximumStaticContentResponses;
    }
    /// Set the maximum number of active static-content responses.
    auto setMaximumStaticContentResponses(unit::ItemCount value) noexcept -> HttpServerOptions & {
        _maximumStaticContentResponses = value;
        return *this;
    }
    /// Get the maximum number of queued blocking static-content operations.
    [[nodiscard]] auto maximumStaticContentOperations() const noexcept -> unit::ItemCount {
        return _maximumStaticContentOperations;
    }
    /// Set the maximum number of queued blocking static-content operations.
    auto setMaximumStaticContentOperations(unit::ItemCount value) noexcept -> HttpServerOptions & {
        _maximumStaticContentOperations = value;
        return *this;
    }
    /// Get the maximum completed static-content bytes awaiting HTTP output.
    [[nodiscard]] auto maximumStaticContentQueueLength() const noexcept -> unit::ByteLength {
        return _maximumStaticContentQueueLength;
    }
    /// Set the maximum completed static-content bytes awaiting HTTP output.
    auto setMaximumStaticContentQueueLength(unit::ByteLength value) noexcept -> HttpServerOptions & {
        _maximumStaticContentQueueLength = value;
        return *this;
    }
    /// Get the maximum aggregate static-content memory retained by active responses.
    [[nodiscard]] auto maximumRetainedStaticContentMemoryLength() const noexcept -> unit::ByteLength {
        return _maximumRetainedStaticContentMemoryLength;
    }
    /// Set the maximum aggregate static-content memory retained by active responses.
    auto setMaximumRetainedStaticContentMemoryLength(unit::ByteLength value) noexcept -> HttpServerOptions & {
        _maximumRetainedStaticContentMemoryLength = value;
        return *this;
    }
    /// Get the static-content output chunk length.
    [[nodiscard]] auto staticContentChunkLength() const noexcept -> unit::ByteLength {
        return _staticContentChunkLength;
    }
    /// Set the positive static-content output chunk length up to 16 KiB.
    auto setStaticContentChunkLength(unit::ByteLength value) noexcept -> HttpServerOptions & {
        _staticContentChunkLength = value;
        return *this;
    }
    /// Get the request-head deadline.
    [[nodiscard]] auto headerTimeout() const noexcept -> time::TimeDelta { return _headerTimeout; }
    /// Set the request-head deadline.
    auto setHeaderTimeout(time::TimeDelta value) noexcept -> HttpServerOptions & {
        _headerTimeout = value;
        return *this;
    }
    /// Get the rolling request-body idle deadline.
    [[nodiscard]] auto bodyIdleTimeout() const noexcept -> time::TimeDelta { return _bodyIdleTimeout; }
    /// Set the rolling request-body idle deadline.
    auto setBodyIdleTimeout(time::TimeDelta value) noexcept -> HttpServerOptions & {
        _bodyIdleTimeout = value;
        return *this;
    }
    /// Get the absolute transaction deadline.
    [[nodiscard]] auto totalTimeout() const noexcept -> time::TimeDelta { return _totalTimeout; }
    /// Set the absolute transaction deadline.
    auto setTotalTimeout(time::TimeDelta value) noexcept -> HttpServerOptions & {
        _totalTimeout = value;
        return *this;
    }
    /// Get the graceful connection-close deadline.
    [[nodiscard]] auto closeTimeout() const noexcept -> time::TimeDelta { return _closeTimeout; }
    /// Set the graceful connection-close deadline.
    auto setCloseTimeout(time::TimeDelta value) noexcept -> HttpServerOptions & {
        _closeTimeout = value;
        return *this;
    }

private:
    HttpHeaderLimits _headerLimits;                                           ///< Request and response header limits.
    unit::ByteLength _maximumStartLineLength{cDefaultMaximumStartLineLength}; ///< Start-line bound.
    unit::ByteLength _maximumBodyLength{cDefaultMaximumBodyLength};           ///< Decoded body hard bound.
    unit::ByteLength _maximumQueueLength{cDefaultMaximumQueueLength};         ///< Codec queue bound.
    unit::ByteLength _maximumFixedResponseLength{cDefaultMaximumFixedResponseLength};        ///< Fixed response bound.
    unit::ItemCount _maximumRequestsPerConnection{cDefaultMaximumRequestsPerConnection};     ///< Reuse bound.
    unit::ItemCount _maximumStaticContentResponses{cDefaultMaximumStaticContentResponses};   ///< Content responses.
    unit::ItemCount _maximumStaticContentOperations{cDefaultMaximumStaticContentOperations}; ///< Worker operations.
    unit::ByteLength _maximumStaticContentQueueLength{cDefaultMaximumStaticContentQueueLength}; ///< Queued chunks.
    unit::ByteLength _maximumRetainedStaticContentMemoryLength{
        cDefaultMaximumRetainedStaticContentMemoryLength};                        ///< Retained static-content memory.
    unit::ByteLength _staticContentChunkLength{cDefaultStaticContentChunkLength}; ///< Output chunk length.
    time::TimeDelta _headerTimeout{cDefaultHeaderTimeout};                        ///< Header deadline.
    time::TimeDelta _bodyIdleTimeout{cDefaultBodyIdleTimeout};                    ///< Body idle deadline.
    time::TimeDelta _totalTimeout{cDefaultTotalTimeout};                          ///< Absolute request deadline.
    time::TimeDelta _closeTimeout{cDefaultCloseTimeout};                          ///< Graceful close deadline.
};

}
