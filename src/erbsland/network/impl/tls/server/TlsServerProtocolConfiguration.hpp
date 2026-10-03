// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../../cryptology/tls/TlsClientAuthentication.hpp"
#include "../../../../cryptology/tls/TlsIdentity_fwd.hpp"
#include "../../../../cryptology/x509/X509TlsCertificatePolicy.hpp"

#include <optional>

namespace erbsland::network::impl {

/// One immutable effective server profile selected as a unit by SNI.
/// @tested{TlsServerProtocolTest TlsServerConnectionTest}
struct TlsServerProtocolConfiguration final {
    /// Create an identity-only server profile with client authentication disabled.
    /// @param value The immutable server identity.
    TlsServerProtocolConfiguration(cryptology::TlsIdentityConstPtr value) : identity{std::move(value)} {}
    /// Create a complete server profile.
    /// @param value The immutable server identity.
    /// @param authentication The client authentication requirement.
    /// @param policy The effective client trust policy.
    TlsServerProtocolConfiguration(
        cryptology::TlsIdentityConstPtr value,
        cryptology::TlsClientAuthentication authentication,
        std::optional<cryptology::X509TlsCertificatePolicy> policy) :
        identity{std::move(value)}, clientAuthentication{authentication}, certificatePolicy{std::move(policy)} {}
    cryptology::TlsIdentityConstPtr identity;                              ///< Server certificate and signer.
    cryptology::TlsClientAuthentication clientAuthentication{
        cryptology::TlsClientAuthentication::Disabled};                    ///< Client authentication requirement.
    std::optional<cryptology::X509TlsCertificatePolicy> certificatePolicy; ///< Effective client trust policy.
};

}
