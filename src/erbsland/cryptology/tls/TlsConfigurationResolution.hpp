// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsConfiguration_fwd.hpp"

#include "../../text/String.hpp"

#include <utility>

namespace erbsland::cryptology {

/// One coherently resolved immutable TLS configuration.
/// @tested{CryptologyConfigurationTest}
class TlsConfigurationResolution final {
public:
    /// Create an empty resolution.
    TlsConfigurationResolution() = default;
    /// Create a successful resolution.
    /// @param requestedLabel The label originally requested.
    /// @param matchedLabel The exact registry entry selected by fallback.
    /// @param configuration The immutable configuration snapshot.
    TlsConfigurationResolution(
        text::String requestedLabel, text::String matchedLabel, TlsConfigurationConstPtr configuration) noexcept :
        _requestedLabel{std::move(requestedLabel)},
        _matchedLabel{std::move(matchedLabel)},
        _configuration{std::move(configuration)} {}

    // defaults
    ~TlsConfigurationResolution() = default;
    TlsConfigurationResolution(const TlsConfigurationResolution &) = default;
    TlsConfigurationResolution(TlsConfigurationResolution &&) noexcept = default;
    auto operator=(const TlsConfigurationResolution &) -> TlsConfigurationResolution & = default;
    auto operator=(TlsConfigurationResolution &&) noexcept -> TlsConfigurationResolution & = default;

public:
    /// Get the label originally requested by the caller.
    [[nodiscard]] auto requestedLabel() const noexcept -> const text::String & { return _requestedLabel; }
    /// Get the exact registered label selected by fallback resolution.
    [[nodiscard]] auto matchedLabel() const noexcept -> const text::String & { return _matchedLabel; }
    /// Get the immutable configuration snapshot.
    [[nodiscard]] auto configuration() const noexcept -> const TlsConfigurationConstPtr & { return _configuration; }

private:
    text::String _requestedLabel;            ///< Original requested label.
    text::String _matchedLabel;              ///< Exact label found in the registry.
    TlsConfigurationConstPtr _configuration; ///< Immutable selected entry.
};

}
