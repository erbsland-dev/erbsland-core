// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsClientAuthentication.hpp"
#include "TlsIdentity.hpp"
#include "TlsTrustConfiguration.hpp"

namespace erbsland::cryptology {

/// The independent server part of an application TLS profile.
/// @tested{CryptologyConfigurationTest TlsConfigurationParserTest}
class TlsServerConfiguration final {
public:
    /// Create an incomplete server configuration requiring an identity before registration.
    TlsServerConfiguration() = default;
    /// Create a server configuration with client authentication disabled.
    /// @param identity The explicitly selected server identity.
    explicit TlsServerConfiguration(TlsIdentity identity) { setIdentity(std::move(identity)); }

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
    /// Get the client certificate authentication requirement.
    [[nodiscard]] auto clientAuthentication() const noexcept -> TlsClientAuthentication {
        return _clientAuthentication;
    }
    /// Set the client certificate authentication requirement.
    void setClientAuthentication(TlsClientAuthentication value) noexcept { _clientAuthentication = value; }

private:
    TlsTrustConfiguration _trust;           ///< Profile trust selection.
    TlsIdentityConstPtr _identity;          ///< Explicit identity, never selected by searching a store.
    TlsClientAuthentication _clientAuthentication{
        TlsClientAuthentication::Disabled}; ///< Client authentication requirement.
};

}
