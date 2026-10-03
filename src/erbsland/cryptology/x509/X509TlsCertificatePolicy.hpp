// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "X509CertificateBundle.hpp"
#include "X509CertificateValidation.hpp"

#include "../../network/Host.hpp"
#include "../../path/Path.hpp"
#include "../../time/Timestamp.hpp"

#include <cstddef>

namespace erbsland::cryptology {

/// Explicit-anchor or system-trust policy for authenticating TLS peer certificates.
/// @seedoc{/reference/cryptology/x509_certificates}
/// @tested{X509TlsCertificatePolicyTest SystemTrustTest}
class X509TlsCertificatePolicy final {
public:
    /// Maximum number of unique target and intermediate certificates, excluding trust anchors.
    static constexpr auto cMaximumCandidateCertificates = std::size_t{256U};
    /// Maximum number of independently indexed trust anchors.
    static constexpr auto cMaximumTrustAnchors = std::size_t{4096U};
    /// Maximum number of certificates in one target-to-anchor path.
    static constexpr auto cMaximumPathDepth = std::size_t{16U};
    /// Maximum number of issuer-edge signature-verification attempts.
    static constexpr auto cMaximumSignatureVerifications = std::size_t{1024U};

public:
    /// Create an explicit-anchor TLS certificate policy.
    /// @param trustAnchors Explicit trusted certificates; an empty bundle causes validation to reject explicitly.
    /// @param intermediates Additional unordered issuer candidates.
    X509TlsCertificatePolicy(X509CertificateBundle trustAnchors, X509CertificateBundle intermediates = {}) noexcept :
        _trustAnchors{std::move(trustAnchors)}, _intermediates{std::move(intermediates)} {}

public: // validation
    /// Validate a peer certificate set at an explicit time.
    /// @param peerCertificates The target certificate first, followed by unordered peer intermediates.
    /// @param referenceIdentity The unresolved DNS name or literal IP address to authenticate.
    /// @param validationTime The time used for every non-anchor validity check.
    /// @return An explicit accepted or rejected validation result.
    [[nodiscard]] auto validateServer(
        const X509CertificateBundle &peerCertificates,
        const network::Host &referenceIdentity,
        time::Timestamp validationTime) const -> X509CertificateValidation;
    /// Validate a peer certificate set at the current time.
    /// @param peerCertificates The target certificate first, followed by unordered peer intermediates.
    /// @param referenceIdentity The unresolved DNS name or literal IP address to authenticate.
    /// @return An explicit accepted or rejected validation result.
    [[nodiscard]] auto validateServer(
        const X509CertificateBundle &peerCertificates, const network::Host &referenceIdentity) const
        -> X509CertificateValidation;

    /// Validate a client certificate chain without DNS or IP identity matching.
    /// @param peerCertificates The client leaf first, followed by untrusted intermediates.
    /// @param validationTime Time used for validity checks.
    /// @return An explicit accepted or rejected result for TLS client authentication.
    [[nodiscard]] auto validateClient(
        const X509CertificateBundle &peerCertificates, time::Timestamp validationTime) const
        -> X509CertificateValidation;
    /// Validate a client certificate chain at the current time.
    /// @param peerCertificates The client leaf first, followed by untrusted intermediates.
    /// @return An explicit accepted or rejected result.
    [[nodiscard]] auto validateClient(const X509CertificateBundle &peerCertificates) const -> X509CertificateValidation;

public: // accessors
    /// Test whether this policy uses operating-system trust.
    [[nodiscard]] auto isSystemTrust() const noexcept -> bool { return _systemTrust; }
    /// Test whether native evaluation may retrieve missing intermediates and revocation information.
    [[nodiscard]] auto allowsNetworkRetrieval() const noexcept -> bool { return _allowNetworkRetrieval; }
    /// Get the explicit Linux CA bundle override, or an empty path for distribution discovery.
    [[nodiscard]] auto systemBundlePath() const noexcept -> const path::Path & { return _systemBundlePath; }
    /// Get the explicit trust anchors.
    [[nodiscard]] auto trustAnchors() const noexcept -> const X509CertificateBundle & { return _trustAnchors; }
    /// Get the configured intermediate certificates.
    [[nodiscard]] auto intermediates() const noexcept -> const X509CertificateBundle & { return _intermediates; }

public: // factories
    /// Create a policy using native macOS/Windows trust or a distribution-maintained Linux CA bundle.
    /// @param allowNetworkRetrieval Permit native intermediate and revocation retrieval; Linux is always offline.
    /// @param bundlePath Explicit Linux CA bundle path; nonempty overrides are rejected on native-trust platforms.
    /// @return The system-trust policy; no native work or file reading is performed during construction.
    [[nodiscard]] static auto systemTrust(bool allowNetworkRetrieval = true, path::Path bundlePath = {})
        -> X509TlsCertificatePolicy;

private:
    X509CertificateBundle _trustAnchors;  ///< Explicit trusted certificate representations.
    X509CertificateBundle _intermediates; ///< Additional unordered issuer candidates.
    bool _systemTrust{};                  ///< Whether system trust is selected.
    bool _allowNetworkRetrieval{true};    ///< Whether native network retrieval is allowed.
    path::Path _systemBundlePath;         ///< Explicit Linux trust-bundle override.
};

}
