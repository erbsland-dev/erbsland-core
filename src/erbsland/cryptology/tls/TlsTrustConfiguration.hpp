// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsRole.hpp"
#include "TlsSettings.hpp"

#include "../CryptologyError.hpp"
#include "../x509/X509TlsCertificatePolicy.hpp"

#include "../../text/Literals.hpp"

#include <optional>

namespace erbsland::cryptology {

/// A profile trust selection resolved against shared TLS settings.
/// @tested{CryptologyConfigurationTest TlsConfigurationParserTest}
class TlsTrustConfiguration final {
public:
    /// Create a trust configuration inheriting application and role defaults.
    TlsTrustConfiguration() = default;
    /// Create an explicit-anchor trust configuration.
    /// @param anchors Trusted root or directly trusted leaf certificates.
    /// @param intermediates Untrusted issuer candidates.
    explicit TlsTrustConfiguration(X509CertificateBundle anchors, X509CertificateBundle intermediates = {}) :
        _anchors{std::move(anchors)}, _intermediates{std::move(intermediates)}, _hasExplicitAnchors{true} {}

public: // accessors
    /// Get the profile override, or no value for inherited selection.
    [[nodiscard]] auto systemTrust() const noexcept -> std::optional<bool> { return _systemTrust; }
    /// Set the profile override.
    void setSystemTrust(std::optional<bool> value) noexcept { _systemTrust = value; }
    /// Get explicit anchors.
    [[nodiscard]] auto trustAnchors() const noexcept -> const X509CertificateBundle & { return _anchors; }
    /// Set explicit anchors, implicitly selecting explicit trust unless overridden.
    void setTrustAnchors(X509CertificateBundle value) {
        _anchors = std::move(value);
        _hasExplicitAnchors = true;
    }
    /// Get additional untrusted intermediate certificates.
    [[nodiscard]] auto intermediates() const noexcept -> const X509CertificateBundle & { return _intermediates; }
    /// Set additional untrusted intermediate certificates.
    void setIntermediates(X509CertificateBundle value) { _intermediates = std::move(value); }

public: // resolution
    /// Resolve a complete policy without native work or file access.
    /// @param settings Shared application settings.
    /// @param role Local connection role.
    /// @param required Whether a usable trust source is required.
    /// @return The effective policy, or no policy when verification is disabled.
    /// @throws CryptologyError If trust settings conflict or required trust is missing.
    [[nodiscard]] auto resolve(const TlsSettings &settings, TlsRole role, bool required = true) const
        -> std::optional<X509TlsCertificatePolicy> {
        using namespace text::literals;
        auto system = settings.systemTrust().value_or(role == TlsRole::Client);
        if (_hasExplicitAnchors) {
            system = false;
        }
        if (_systemTrust.has_value()) {
            system = *_systemTrust;
        }
        if (system && _hasExplicitAnchors) {
            throw CryptologyError{"System and explicit TLS trust must not be combined."_el};
        }
        if (!_intermediates.isEmpty() && !_hasExplicitAnchors) {
            throw CryptologyError{"TLS intermediate certificates require explicit trust anchors."_el};
        }
        if (!required) {
            return std::nullopt;
        }
        if (system) {
            return X509TlsCertificatePolicy::systemTrust(settings.allowsTrustNetwork(), settings.trustBundlePath());
        }
        if (_anchors.isEmpty()) {
            throw CryptologyError{
                "TLS certificate verification requires explicit trust anchors when system trust is disabled."_el};
        }
        return X509TlsCertificatePolicy{_anchors, _intermediates};
    }

private:
    std::optional<bool> _systemTrust;     ///< Explicit profile selection.
    X509CertificateBundle _anchors;       ///< Explicit trusted certificates.
    X509CertificateBundle _intermediates; ///< Untrusted issuer candidates.
    bool _hasExplicitAnchors{};           ///< Distinguish unspecified anchors from an explicitly empty bundle.
};

}
