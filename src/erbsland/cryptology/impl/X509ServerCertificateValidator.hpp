// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "X509ServerCertificateValidator_fwd.hpp"

#include "../x509/X509CertificateValidation.hpp"
#include "../x509/X509ServerCertificatePolicy.hpp"

#include "../../network/Host.hpp"
#include "../../text/String.hpp"
#include "../../time/DateTime.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace erbsland::cryptology::impl {

/// Bounded portable path builder and validator used by `X509ServerCertificatePolicy`.
/// Path construction follows RFC 4158 sections 2.4 and 5; validation follows RFC 5280 sections 4.1.2.5,
/// 4.2.1.3, 4.2.1.9, 4.2.1.12, and 6.1; TLS purpose follows RFC 8446 section 4.4.2.2; and service identity matching
/// follows RFC 9525 section 6 and RFC 9549 using strict IDNA2008 A-label comparison.
/// All retained certificate and signature material is public. This implementation creates no secret intermediate and
/// therefore has no secret state requiring erasure.
/// @notest{Tested through the public X509ServerCertificatePolicy interface.}
class X509ServerCertificateValidator final {
private:
    /// One available path-building certificate and its trust-anchor status.
    struct Candidate final {
        X509Certificate certificate;
        bool isAnchor{};
    };

    /// Verification result cached for one certificate-to-issuer edge.
    enum class EdgeStatus : uint8_t {
        Valid,
        Invalid,
        Unsupported,
    };

    /// Cached canonical DER identity and verification result for one edge.
    struct VerifiedEdge final {
        mem::ByteBlock childDer;
        mem::ByteBlock issuerDer;
        EdgeStatus status{};
    };

    /// Parsed DNS service identity labels and wildcard flag.
    struct DnsName final {
        std::vector<text::String> labels;
        bool hasWildcard{};
    };

public:
    /// Create a validator for one policy, peer chain, identity, and point in time.
    X509ServerCertificateValidator(
        const X509ServerCertificatePolicy &policy,
        const X509CertificateBundle &peerCertificates,
        const network::Host &referenceIdentity,
        time::DateTime validationTime) noexcept :
        _policy{policy},
        _peerCertificates{peerCertificates},
        _referenceIdentity{referenceIdentity},
        _validationTime{validationTime} {}

public:
    /// Build and validate the best certificate path.
    [[nodiscard]] auto validate() -> X509CertificateValidation;

private: // RFC 4158 path construction
    /// Collect peer and trust-store certificates within resource bounds.
    [[nodiscard]] auto collectCandidates() -> bool;
    /// Add one certificate candidate with its trust-anchor status.
    void addCandidate(const X509Certificate &certificate, bool isAnchor);
    /// Build and validate a path beginning with one child certificate.
    [[nodiscard]] auto buildPath(const X509Certificate &child) -> std::optional<X509CertificateValidation>;
    /// Test whether a certificate is a configured trust anchor.
    [[nodiscard]] auto isExplicitAnchor(const X509Certificate &certificate) const -> bool;
    /// Test whether a certificate already occurs in the candidate path.
    [[nodiscard]] auto isInCurrentPath(const X509Certificate &certificate) const -> bool;
    /// Test whether issuer metadata and signature data permit one edge.
    [[nodiscard]] static auto hasIssuerLink(const X509Certificate &child, const X509Certificate &issuer) -> bool;
    /// Verify and cache one child-to-issuer certificate edge.
    [[nodiscard]] auto verifyEdge(const X509Certificate &child, const X509Certificate &issuer) -> EdgeStatus;

private: // RFC 5280 path validation and RFC 8446 purpose validation
    /// Validate the currently selected chain and server-authentication purpose.
    [[nodiscard]] auto validateSelectedPath() -> bool;
    /// Validate one non-anchor certificate's common constraints.
    [[nodiscard]] auto validateNonAnchorCertificate(const X509Certificate &certificate) -> bool;
    /// Validate required handling of critical certificate extensions.
    [[nodiscard]] auto validateCriticalExtensions(const X509Certificate &certificate) -> bool;
    /// Validate TLS server requirements on the target certificate.
    [[nodiscard]] auto validateTarget(const X509Certificate &certificate) -> bool;
    /// Validate CA constraints on one intermediate certificate.
    [[nodiscard]] auto validateIntermediate(const X509Certificate &certificate, std::size_t pathIndex) -> bool;
    /// Test whether BasicConstraints is present and critical.
    [[nodiscard]] static auto hasCriticalBasicConstraints(const X509Certificate &certificate) -> bool;
    /// Test whether extended key usage permits TLS server authentication.
    [[nodiscard]] static auto permitsServerAuthentication(const X509Certificate &certificate) -> bool;
    /// Count subordinate CA certificates below one selected path position.
    [[nodiscard]] auto subordinateCaCount(std::size_t pathIndex) const -> std::size_t;
    /// Test whether issuer and subject identify the same certificate entity.
    [[nodiscard]] static auto isSelfIssued(const X509Certificate &certificate) -> bool;

private: // RFC 9525 service-identity matching
    /// Validate the configured service identity against the target certificate.
    [[nodiscard]] auto validateReferenceIdentity() -> bool;
    /// Test whether a certificate presents the configured server identity.
    [[nodiscard]] auto matchServerIdentity(const X509Certificate &certificate) -> bool;
    /// Parse and canonicalize a DNS identity, optionally permitting a wildcard label.
    [[nodiscard]] static auto parseDnsNameOrThrow(const text::String &name, bool allowWildcard) -> DnsName;
    /// Test whether a presented DNS identity matches the reference identity.
    [[nodiscard]] static auto dnsNameMatches(const DnsName &reference, const DnsName &presented) -> bool;

private: // structured diagnostics
    /// Record the most useful certificate-validation failure seen so far.
    void recordFailure(
        X509CertificateValidationFailureCategory category,
        std::optional<X509Certificate> certificate,
        const text::String &diagnostic,
        std::optional<X509Certificate> issuerCandidate = {});
    /// Produce a rejected result from the accumulated best failure.
    [[nodiscard]] auto rejectedResult() const -> X509CertificateValidation;
    /// Produce one immediate rejected result and diagnostic.
    [[nodiscard]] auto directRejection(
        X509CertificateValidationFailureCategory category,
        std::optional<X509Certificate> certificate,
        const text::String &diagnostic) const -> X509CertificateValidation;

private:
    const X509ServerCertificatePolicy &_policy;
    const X509CertificateBundle &_peerCertificates;
    const network::Host &_referenceIdentity;
    time::DateTime _validationTime;
    std::vector<Candidate> _candidates;
    util::List<X509Certificate> _path;
    std::vector<VerifiedEdge> _verifiedEdges;
    std::size_t _signatureVerificationCount{};
    std::size_t _bestFailureDepth{};
    bool _resourceLimitExhausted{};
    std::optional<text::String> _invalidPresentedIdentityDiagnostic;
    std::optional<X509CertificateValidationFailure> _bestFailure;
};

}
