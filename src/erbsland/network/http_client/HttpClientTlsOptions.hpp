// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../source/SocketBufferLimits.hpp"

#include "../../text/String.hpp"
#include "../../time/TimeDelta.hpp"

#include <utility>

namespace erbsland::network {

/// Curated TLS policy for HTTPS client requests.
/// @tested{HttpClientTest}
class HttpClientTlsOptions final {
public:
    /// Default client TLS configuration label.
    inline static const auto cDefaultConfigurationLabel = text::String{"http/client"};
    /// Default TLS handshake deadline.
    inline static const auto cDefaultHandshakeTimeout = time::TimeDelta::seconds(30);
    /// Default authenticated transport idle deadline.
    inline static const auto cDefaultIdleTimeout = time::TimeDelta::minutes(5);

public:
    /// Get the TLS configuration label.
    [[nodiscard]] auto configurationLabel() const noexcept -> const text::String & { return _configurationLabel; }
    /// Set the TLS configuration label.
    auto setConfigurationLabel(text::String value) noexcept -> HttpClientTlsOptions & {
        _configurationLabel = std::move(value);
        return *this;
    }
    /// Get the TLS protocol buffer limits.
    [[nodiscard]] auto bufferLimits() const noexcept -> SocketBufferLimits { return _bufferLimits; }
    /// Set the TLS protocol buffer limits.
    auto setBufferLimits(SocketBufferLimits value) noexcept -> HttpClientTlsOptions & {
        _bufferLimits = value;
        return *this;
    }
    /// Get the TLS handshake deadline.
    [[nodiscard]] auto handshakeTimeout() const noexcept -> time::TimeDelta { return _handshakeTimeout; }
    /// Set the TLS handshake deadline.
    auto setHandshakeTimeout(time::TimeDelta value) noexcept -> HttpClientTlsOptions & {
        _handshakeTimeout = value;
        return *this;
    }
    /// Get the authenticated transport idle deadline.
    [[nodiscard]] auto idleTimeout() const noexcept -> time::TimeDelta { return _idleTimeout; }
    /// Set the authenticated transport idle deadline.
    auto setIdleTimeout(time::TimeDelta value) noexcept -> HttpClientTlsOptions & {
        _idleTimeout = value;
        return *this;
    }

private:
    text::String _configurationLabel{cDefaultConfigurationLabel}; ///< Registry label.
    SocketBufferLimits _bufferLimits;                             ///< TLS queues.
    time::TimeDelta _handshakeTimeout{cDefaultHandshakeTimeout};  ///< Handshake deadline.
    time::TimeDelta _idleTimeout{cDefaultIdleTimeout};            ///< Authenticated idle deadline.
};

}
