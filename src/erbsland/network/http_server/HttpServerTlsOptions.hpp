// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../tls/TlsConfigurationMapping.hpp"

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
    inline static const auto cDefaultConfigurationLabel = text::String{"http"};
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
    /// Get exact canonical SNI configuration mappings.
    [[nodiscard]] auto configurationMappings() const noexcept -> const std::vector<TlsConfigurationMapping> & {
        return _configurationMappings;
    }
    /// Replace exact canonical SNI configuration mappings.
    auto setConfigurationMappings(std::vector<TlsConfigurationMapping> value) noexcept -> HttpServerTlsOptions & {
        _configurationMappings = std::move(value);
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
    std::vector<TlsConfigurationMapping> _configurationMappings;  ///< Exact SNI configuration mappings.
    unit::ItemCount _maximumConcurrentHandshakes{cDefaultMaximumConcurrentHandshakes}; ///< Handshake quota.
    time::TimeDelta _handshakeTimeout{cDefaultHandshakeTimeout};                       ///< Handshake deadline.
};

}
