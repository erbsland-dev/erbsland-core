// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpConnectionInfo_fwd.hpp"
#include "HttpTlsConnectionInfo.hpp"

#include "../IpEndpoint.hpp"

#include <optional>
#include <utility>

namespace erbsland::network {

/// An immutable transport-independent snapshot of an HTTP connection.
/// The snapshot deliberately exposes no transport object or mutable event interface and can be extended for future
/// HTTP transports without changing the request ownership model.
/// @tested{HttpServerLiveTest}
class HttpConnectionInfo final {
public:
    /// Create a connection information snapshot.
    HttpConnectionInfo(
        std::optional<IpEndpoint> localEndpoint,
        std::optional<IpEndpoint> remoteEndpoint,
        std::optional<HttpTlsConnectionInfo> tls = {}) :
        _localEndpoint{std::move(localEndpoint)}, _remoteEndpoint{std::move(remoteEndpoint)}, _tls{std::move(tls)} {}

public:
    /// Get the local endpoint captured for this connection.
    [[nodiscard]] auto localEndpoint() const noexcept -> const std::optional<IpEndpoint> & { return _localEndpoint; }
    /// Get the remote endpoint captured for this connection.
    [[nodiscard]] auto remoteEndpoint() const noexcept -> const std::optional<IpEndpoint> & { return _remoteEndpoint; }
    /// Test whether this connection has authenticated TLS information.
    [[nodiscard]] auto isSecure() const noexcept -> bool { return _tls.has_value(); }
    /// Get the negotiated TLS information, if this is a secure connection.
    [[nodiscard]] auto tls() const noexcept -> const std::optional<HttpTlsConnectionInfo> & { return _tls; }

private:
    std::optional<IpEndpoint> _localEndpoint;  ///< Local transport endpoint.
    std::optional<IpEndpoint> _remoteEndpoint; ///< Remote transport endpoint.
    std::optional<HttpTlsConnectionInfo> _tls; ///< Negotiated TLS details, if available.
};

}
