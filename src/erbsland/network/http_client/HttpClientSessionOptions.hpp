// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpCookieJarOptions.hpp"

#include "../http/HttpHeaderLimits.hpp"
#include "../source/SocketBufferLimits.hpp"

#include "../../time/TimeDelta.hpp"
#include "../../unit/ByteLength.hpp"
#include "../../unit/ItemCount.hpp"

namespace erbsland::network {

/// Limits and deadlines captured for submitted HTTP client requests.
/// @tested{HttpClientTest}
class HttpClientSessionOptions final {
public:
    static constexpr auto cDefaultMaximumConcurrentRequests = unit::ItemCount{8U}; ///< Active requests.
    static constexpr auto cDefaultMaximumPendingRequests = unit::ItemCount{256U};  ///< Queued requests.
    static constexpr auto cDefaultMaximumRetainedRequestBodyLength =
        unit::ByteLength{64U * 1024U * 1024U};                                     ///< Session fixed-body quota.
    static constexpr auto cDefaultMaximumBodyLength =
        unit::ByteLength{16U * 1024U * 1024U};                                     ///< Decoded response bound.
    static constexpr auto cDefaultMaximumQueueLength =
        unit::ByteLength{1024U * 1024U};                                           ///< Per-transaction queue bound.
    static constexpr auto cDefaultMaximumStartLineLength = unit::ByteLength{8U * 1024U};     ///< Status-line bound.
    static constexpr auto cDefaultMaximumIdleConnections = unit::ItemCount{8U};              ///< Global idle cap.
    static constexpr auto cDefaultMaximumTransactionsPerConnection = unit::ItemCount{1000U}; ///< Reuse cap.
    inline static const auto cDefaultDnsTimeout = time::TimeDelta::seconds(10);              ///< DNS deadline.
    inline static const auto cDefaultConnectTimeout = time::TimeDelta::seconds(30);          ///< TCP deadline.
    inline static const auto cDefaultHeaderTimeout = time::TimeDelta::seconds(30);         ///< Response-head deadline.
    inline static const auto cDefaultBodyIdleTimeout = time::TimeDelta::minutes(5);        ///< Body idle deadline.
    inline static const auto cDefaultOverallTimeout = time::TimeDelta::minutes(30);        ///< Overall deadline.
    inline static const auto cDefaultCloseTimeout = time::TimeDelta::seconds(10);          ///< Close deadline.
    inline static const auto cDefaultIdleConnectionTimeout = time::TimeDelta::seconds(30); ///< Idle reuse deadline.

public:                                                                                    // accessors
    /// Get the maximum concurrent request count.
    [[nodiscard]] auto maximumConcurrentRequests() const noexcept -> unit::ItemCount { return _maximumConcurrent; }
    /// Set the maximum concurrent request count.
    auto setMaximumConcurrentRequests(unit::ItemCount value) noexcept -> HttpClientSessionOptions & {
        _maximumConcurrent = value;
        return *this;
    }
    /// Get the maximum pending request count.
    [[nodiscard]] auto maximumPendingRequests() const noexcept -> unit::ItemCount { return _maximumPending; }
    /// Set the maximum pending request count.
    auto setMaximumPendingRequests(unit::ItemCount value) noexcept -> HttpClientSessionOptions & {
        _maximumPending = value;
        return *this;
    }
    /// Get the aggregate retained fixed-body quota.
    [[nodiscard]] auto maximumRetainedRequestBodyLength() const noexcept -> unit::ByteLength {
        return _maximumRetainedBody;
    }
    /// Set the aggregate retained fixed-body quota.
    auto setMaximumRetainedRequestBodyLength(unit::ByteLength value) noexcept -> HttpClientSessionOptions & {
        _maximumRetainedBody = value;
        return *this;
    }
    /// Get the decoded response hard maximum.
    [[nodiscard]] auto maximumBodyLength() const noexcept -> unit::ByteLength { return _maximumBody; }
    /// Set the decoded response hard maximum.
    auto setMaximumBodyLength(unit::ByteLength value) noexcept -> HttpClientSessionOptions & {
        _maximumBody = value;
        return *this;
    }
    /// Get the transaction queue maximum.
    [[nodiscard]] auto maximumQueueLength() const noexcept -> unit::ByteLength { return _maximumQueue; }
    /// Set the transaction queue maximum.
    auto setMaximumQueueLength(unit::ByteLength value) noexcept -> HttpClientSessionOptions & {
        _maximumQueue = value;
        return *this;
    }
    /// Get the response status-line maximum.
    [[nodiscard]] auto maximumStartLineLength() const noexcept -> unit::ByteLength { return _maximumStartLine; }
    /// Set the response status-line maximum.
    auto setMaximumStartLineLength(unit::ByteLength value) noexcept -> HttpClientSessionOptions & {
        _maximumStartLine = value;
        return *this;
    }
    /// Get the request/response header limits.
    [[nodiscard]] auto headerLimits() const noexcept -> HttpHeaderLimits { return _headerLimits; }
    /// Set the request/response header limits.
    auto setHeaderLimits(HttpHeaderLimits value) noexcept -> HttpClientSessionOptions & {
        _headerLimits = value;
        return *this;
    }
    /// Get connected socket buffer limits.
    [[nodiscard]] auto socketBufferLimits() const noexcept -> SocketBufferLimits { return _socketBufferLimits; }
    /// Set connected socket buffer limits.
    auto setSocketBufferLimits(SocketBufferLimits value) noexcept -> HttpClientSessionOptions & {
        _socketBufferLimits = value;
        return *this;
    }
    /// Test whether automatic cookie storage and generation are enabled.
    [[nodiscard]] auto automaticCookiesEnabled() const noexcept -> bool { return _automaticCookies; }
    /// Enable or disable automatic cookie storage and generation.
    auto setAutomaticCookiesEnabled(const bool value) noexcept -> HttpClientSessionOptions & {
        _automaticCookies = value;
        return *this;
    }
    /// Get cookie-jar limits captured for subsequently submitted requests.
    [[nodiscard]] auto cookieJarOptions() const noexcept -> HttpCookieJarOptions { return _cookieJarOptions; }
    /// Set cookie-jar limits.
    auto setCookieJarOptions(HttpCookieJarOptions value) noexcept -> HttpClientSessionOptions & {
        _cookieJarOptions = value;
        return *this;
    }
    /// Test whether completed HTTP/1.1 connections may be reused sequentially.
    [[nodiscard]] auto connectionReuseEnabled() const noexcept -> bool { return _connectionReuse; }
    /// Enable or disable sequential connection reuse.
    auto setConnectionReuseEnabled(const bool value) noexcept -> HttpClientSessionOptions & {
        _connectionReuse = value;
        return *this;
    }
    /// Get the global idle-connection cap. At most one idle connection is retained per origin.
    [[nodiscard]] auto maximumIdleConnections() const noexcept -> unit::ItemCount { return _maximumIdleConnections; }
    /// Set the global idle-connection cap.
    auto setMaximumIdleConnections(const unit::ItemCount value) noexcept -> HttpClientSessionOptions & {
        _maximumIdleConnections = value;
        return *this;
    }
    /// Get the idle connection deadline.
    [[nodiscard]] auto idleConnectionTimeout() const noexcept -> time::TimeDelta { return _idleConnectionTimeout; }
    /// Set the idle connection deadline.
    auto setIdleConnectionTimeout(const time::TimeDelta value) noexcept -> HttpClientSessionOptions & {
        _idleConnectionTimeout = value;
        return *this;
    }
    /// Get the transaction cap for one physical connection.
    [[nodiscard]] auto maximumTransactionsPerConnection() const noexcept -> unit::ItemCount {
        return _maximumTransactionsPerConnection;
    }
    /// Set the transaction cap for one physical connection.
    auto setMaximumTransactionsPerConnection(const unit::ItemCount value) noexcept -> HttpClientSessionOptions & {
        _maximumTransactionsPerConnection = value;
        return *this;
    }
    /// Get the DNS deadline.
    [[nodiscard]] auto dnsTimeout() const noexcept -> time::TimeDelta { return _dnsTimeout; }
    /// Set the DNS deadline.
    auto setDnsTimeout(time::TimeDelta value) noexcept -> HttpClientSessionOptions & {
        _dnsTimeout = value;
        return *this;
    }
    /// Get the TCP connection deadline after resolution.
    [[nodiscard]] auto connectTimeout() const noexcept -> time::TimeDelta { return _connectTimeout; }
    /// Set the TCP connection deadline after resolution.
    auto setConnectTimeout(time::TimeDelta value) noexcept -> HttpClientSessionOptions & {
        _connectTimeout = value;
        return *this;
    }
    /// Get the response-head deadline.
    [[nodiscard]] auto headerTimeout() const noexcept -> time::TimeDelta { return _headerTimeout; }
    /// Set the response-head deadline.
    auto setHeaderTimeout(time::TimeDelta value) noexcept -> HttpClientSessionOptions & {
        _headerTimeout = value;
        return *this;
    }
    /// Get the response-body idle deadline.
    [[nodiscard]] auto bodyIdleTimeout() const noexcept -> time::TimeDelta { return _bodyIdleTimeout; }
    /// Set the response-body idle deadline.
    auto setBodyIdleTimeout(time::TimeDelta value) noexcept -> HttpClientSessionOptions & {
        _bodyIdleTimeout = value;
        return *this;
    }
    /// Get the overall request deadline starting at submission.
    [[nodiscard]] auto overallTimeout() const noexcept -> time::TimeDelta { return _overallTimeout; }
    /// Set the overall request deadline starting at submission.
    auto setOverallTimeout(time::TimeDelta value) noexcept -> HttpClientSessionOptions & {
        _overallTimeout = value;
        return *this;
    }
    /// Get the graceful connection-close deadline.
    [[nodiscard]] auto closeTimeout() const noexcept -> time::TimeDelta { return _closeTimeout; }
    /// Set the graceful connection-close deadline.
    auto setCloseTimeout(time::TimeDelta value) noexcept -> HttpClientSessionOptions & {
        _closeTimeout = value;
        return *this;
    }

private:
    unit::ItemCount _maximumConcurrent{cDefaultMaximumConcurrentRequests};           ///< Active request cap.
    unit::ItemCount _maximumPending{cDefaultMaximumPendingRequests};                 ///< Pending request cap.
    unit::ByteLength _maximumRetainedBody{cDefaultMaximumRetainedRequestBodyLength}; ///< Fixed-body quota.
    unit::ByteLength _maximumBody{cDefaultMaximumBodyLength};                        ///< Response bound.
    unit::ByteLength _maximumQueue{cDefaultMaximumQueueLength};                      ///< Codec queue bound.
    unit::ByteLength _maximumStartLine{cDefaultMaximumStartLineLength};              ///< Status-line bound.
    HttpHeaderLimits _headerLimits;                                                  ///< Header bounds.
    SocketBufferLimits _socketBufferLimits;                                          ///< Socket queue bounds.
    HttpCookieJarOptions _cookieJarOptions;                                          ///< Cookie storage bounds.
    time::TimeDelta _dnsTimeout{cDefaultDnsTimeout};                                 ///< DNS deadline.
    time::TimeDelta _connectTimeout{cDefaultConnectTimeout};                         ///< TCP deadline.
    time::TimeDelta _headerTimeout{cDefaultHeaderTimeout};                           ///< Head deadline.
    time::TimeDelta _bodyIdleTimeout{cDefaultBodyIdleTimeout};                       ///< Body idle deadline.
    time::TimeDelta _overallTimeout{cDefaultOverallTimeout};                         ///< Overall deadline.
    time::TimeDelta _closeTimeout{cDefaultCloseTimeout};                             ///< Close deadline.
    time::TimeDelta _idleConnectionTimeout{cDefaultIdleConnectionTimeout};           ///< Idle reuse deadline.
    unit::ItemCount _maximumIdleConnections{cDefaultMaximumIdleConnections};         ///< Global idle cap.
    unit::ItemCount _maximumTransactionsPerConnection{cDefaultMaximumTransactionsPerConnection}; ///< Reuse cap.
    bool _automaticCookies{true};                                                                ///< Cookie automation.
    bool _connectionReuse{true};                                                                 ///< Sequential reuse.
};

}
