// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsClientAuthentication.hpp"
#include "TlsIdentity.hpp"
#include "TlsTrustConfiguration.hpp"

namespace erbsland::cryptology {

/// The independent client part of an application TLS profile.
/// @tested{CryptologyConfigurationTest TlsConfigurationParserTest}
class TlsClientConfiguration final {
public:
    /// Create a client configuration inheriting trust defaults and without an identity.
    TlsClientConfiguration() = default;
    /// Create a client configuration with explicit trust settings and optional identity.
    /// @param trust Profile trust settings.
    /// @param identity Optional immutable client identity.
    explicit TlsClientConfiguration(TlsTrustConfiguration trust, TlsIdentityConstPtr identity = {}) :
        _trust{std::move(trust)}, _identity{std::move(identity)} {}
    /// Create a client configuration with an identity and inherited trust.
    /// @param identity The explicitly selected client identity.
    explicit TlsClientConfiguration(TlsIdentity identity) { setIdentity(std::move(identity)); }

public: // accessors
    /// Get profile trust settings.
    [[nodiscard]] auto trust() const noexcept -> const TlsTrustConfiguration & { return _trust; }
    /// Replace profile trust settings.
    void setTrust(TlsTrustConfiguration value) { _trust = std::move(value); }
    /// Get the explicitly selected certificate identity, or null when absent.
    [[nodiscard]] auto identity() const noexcept -> const TlsIdentityConstPtr & { return _identity; }
    /// Set the certificate identity.
    void setIdentity(TlsIdentity value) { _identity = std::make_shared<const TlsIdentity>(std::move(value)); }
    /// Share an immutable certificate identity.
    void setIdentity(TlsIdentityConstPtr value) { _identity = std::move(value); }

private:
    TlsTrustConfiguration _trust;  ///< Profile trust selection.
    TlsIdentityConstPtr _identity; ///< Explicit identity, never selected by searching a store.
};

}
