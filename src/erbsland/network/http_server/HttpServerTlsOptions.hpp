// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../tls/TlsServerIdentityMapping.hpp"

#include "../../text/String.hpp"
#include "../../time/TimeDelta.hpp"
#include "../../unit/ItemCount.hpp"

#include <utility>
#include <vector>

namespace erbsland::network {

/// Curated HTTPS configuration captured when an HTTP server starts.
/// @tested{HttpServerLiveTest}
class HttpServerTlsOptions final {
public:
    /// Default server-identity configuration label.
    inline static const auto cDefaultConfigurationLabel = text::String{"http/server"};
    /// Default incomplete-handshake capacity.
    static constexpr auto cDefaultMaximumConcurrentHandshakes = unit::ItemCount{128U};
    /// Default TLS handshake deadline.
    inline static const auto cDefaultHandshakeTimeout = time::TimeDelta::seconds(30);

public:
    /// Get the default server-identity configuration label.
    [[nodiscard]] auto configurationLabel() const noexcept -> const text::String & { return _configurationLabel; }
    /// Set the default server-identity configuration label.
    auto setConfigurationLabel(text::String value) noexcept -> HttpServerTlsOptions & {
        _configurationLabel = std::move(value);
        return *this;
    }
    /// Get exact canonical SNI identity mappings.
    [[nodiscard]] auto identityMappings() const noexcept -> const std::vector<TlsServerIdentityMapping> & {
        return _identityMappings;
    }
    /// Replace exact canonical SNI identity mappings.
    auto setIdentityMappings(std::vector<TlsServerIdentityMapping> value) noexcept -> HttpServerTlsOptions & {
        _identityMappings = std::move(value);
        return *this;
    }
    /// Get the maximum number of concurrent incomplete handshakes.
    [[nodiscard]] auto maximumConcurrentHandshakes() const noexcept -> unit::ItemCount {
        return _maximumConcurrentHandshakes;
    }
    /// Set the positive finite number of concurrent incomplete handshakes.
    auto setMaximumConcurrentHandshakes(unit::ItemCount value) noexcept -> HttpServerTlsOptions & {
        _maximumConcurrentHandshakes = value;
        return *this;
    }
    /// Get the TLS handshake deadline.
    [[nodiscard]] auto handshakeTimeout() const noexcept -> time::TimeDelta { return _handshakeTimeout; }
    /// Set the positive TLS handshake deadline.
    auto setHandshakeTimeout(time::TimeDelta value) noexcept -> HttpServerTlsOptions & {
        _handshakeTimeout = value;
        return *this;
    }

private:
    text::String _configurationLabel{cDefaultConfigurationLabel}; ///< Default identity registry label.
    std::vector<TlsServerIdentityMapping> _identityMappings;      ///< Exact SNI identity mappings.
    unit::ItemCount _maximumConcurrentHandshakes{cDefaultMaximumConcurrentHandshakes}; ///< Handshake quota.
    time::TimeDelta _handshakeTimeout{cDefaultHandshakeTimeout};                       ///< Handshake deadline.
};

}
