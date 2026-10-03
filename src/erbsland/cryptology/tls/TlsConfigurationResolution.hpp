// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsConfiguration_fwd.hpp"
#include "TlsRole.hpp"

#include "../x509/X509TlsCertificatePolicy.hpp"

#include "../../text/String.hpp"

#include <optional>
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
    /// @param matchedLabel The exact profile or built-in default selected by fallback.
    /// @param role The selected local role.
    /// @param policy The effective certificate policy.
    /// @param configuration The immutable configuration snapshot.
    TlsConfigurationResolution(
        text::String requestedLabel,
        text::String matchedLabel,
        TlsConfigurationConstPtr configuration,
        TlsRole role,
        std::optional<X509TlsCertificatePolicy> policy) noexcept :
        _requestedLabel{std::move(requestedLabel)},
        _matchedLabel{std::move(matchedLabel)},
        _configuration{std::move(configuration)},
        _role{role},
        _certificatePolicy{std::move(policy)} {}

    // defaults
    ~TlsConfigurationResolution() = default;
    TlsConfigurationResolution(const TlsConfigurationResolution &) = default;
    TlsConfigurationResolution(TlsConfigurationResolution &&) noexcept = default;
    auto operator=(const TlsConfigurationResolution &) -> TlsConfigurationResolution & = default;
    auto operator=(TlsConfigurationResolution &&) noexcept -> TlsConfigurationResolution & = default;

public:
    /// Get the label originally requested by the caller.
    [[nodiscard]] auto requestedLabel() const noexcept -> const text::String & { return _requestedLabel; }
    /// Get the exact profile label or empty built-in default label selected by fallback resolution.
    [[nodiscard]] auto matchedLabel() const noexcept -> const text::String & { return _matchedLabel; }
    /// Get the immutable configuration snapshot.
    [[nodiscard]] auto configuration() const noexcept -> const TlsConfigurationConstPtr & { return _configuration; }

    /// Get the selected local TLS role.
    [[nodiscard]] auto role() const noexcept -> TlsRole { return _role; }
    /// Get the effective certificate policy, absent when client authentication is disabled.
    [[nodiscard]] auto certificatePolicy() const noexcept -> const std::optional<X509TlsCertificatePolicy> & {
        return _certificatePolicy;
    }

private:
    text::String _requestedLabel;                               ///< Original requested label.
    text::String _matchedLabel;                                 ///< Exact label found in the registry.
    TlsConfigurationConstPtr _configuration;                    ///< Immutable selected entry.
    TlsRole _role{TlsRole::Client};                             ///< Selected role.
    std::optional<X509TlsCertificatePolicy> _certificatePolicy; ///< Captured effective trust policy.
};

}
