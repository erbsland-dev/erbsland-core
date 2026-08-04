// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "X509CertificateBundle.hpp"
#include "X509CertificateValidation.hpp"

#include "../../network/Host.hpp"
#include "../../time/DateTime.hpp"

#include <cstddef>

namespace erbsland::cryptology {

/// A portable explicit-anchor policy for authenticating TLS server certificates.
/// Path building follows RFC 4158 sections 2.4 and 5. Path validation follows RFC 5280 sections 4.1.2.5,
/// 4.2.1.3, 4.2.1.9, 4.2.1.12, and 6.1. TLS purpose checks follow RFC 8446 section 4.4.2.2, and DNS/IP identity
/// matching follows RFC 9525 sections 6.1--6.6. Revocation, name/policy constraints, RFC 4518 name equivalence,
/// platform trust stores, and UTS #46 compatibility mapping are not performed. DNS references use strict IDNA2008;
/// presented certificate dNSName values remain canonical ASCII A-labels for comparison.
/// @seedoc{/reference/cryptology/x509_certificates}
/// @tested{X509ServerCertificatePolicyTest}
class X509ServerCertificatePolicy final {
public:
    /// Maximum aggregate number of unique target, intermediate, and anchor certificates.
    static constexpr auto cMaximumCandidateCertificates = std::size_t{256U};
    /// Maximum number of certificates in one target-to-anchor path.
    static constexpr auto cMaximumPathDepth = std::size_t{16U};
    /// Maximum number of issuer-edge signature-verification attempts.
    static constexpr auto cMaximumSignatureVerifications = std::size_t{1024U};

public:
    /// Create a server-authentication policy.
    /// @param trustAnchors Explicit trusted certificates; an empty bundle causes validation to reject explicitly.
    /// @param intermediates Additional unordered issuer candidates.
    X509ServerCertificatePolicy(X509CertificateBundle trustAnchors, X509CertificateBundle intermediates = {}) noexcept :
        _trustAnchors{std::move(trustAnchors)}, _intermediates{std::move(intermediates)} {}

public: // validation
    /// Validate a peer certificate set at an explicit time.
    /// @param peerCertificates The target certificate first, followed by unordered peer intermediates.
    /// @param referenceIdentity The unresolved DNS name or literal IP address to authenticate.
    /// @param validationTime The time used for every non-anchor validity check.
    /// @return An explicit accepted or rejected validation result.
    [[nodiscard]] auto validate(
        const X509CertificateBundle &peerCertificates,
        const network::Host &referenceIdentity,
        time::DateTime validationTime) const -> X509CertificateValidation;
    /// Validate a peer certificate set at the current time.
    /// @param peerCertificates The target certificate first, followed by unordered peer intermediates.
    /// @param referenceIdentity The unresolved DNS name or literal IP address to authenticate.
    /// @return An explicit accepted or rejected validation result.
    [[nodiscard]] auto validate(
        const X509CertificateBundle &peerCertificates, const network::Host &referenceIdentity) const
        -> X509CertificateValidation;

public: // accessors
    /// Get the explicit trust anchors.
    [[nodiscard]] auto trustAnchors() const noexcept -> const X509CertificateBundle & { return _trustAnchors; }
    /// Get the configured intermediate certificates.
    [[nodiscard]] auto intermediates() const noexcept -> const X509CertificateBundle & { return _intermediates; }

private:
    X509CertificateBundle _trustAnchors;  ///< Explicit trusted certificate representations.
    X509CertificateBundle _intermediates; ///< Additional unordered issuer candidates.
};

}
