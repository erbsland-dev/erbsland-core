// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "X509TlsCertificateValidator.hpp"

#include "../../err/ParseError.hpp"
#include "../../text/Literals.hpp"
#include "../../text/punycode/PunycodeDecoder.hpp"
#include "../../text/punycode/PunycodeEncoder.hpp"
#include "../../text/StringCharReader.hpp"
#include "../../text/StringEditor.hpp"

#include <compare>
#include <exception>

namespace erbsland::cryptology::impl {

using namespace text::literals;

auto X509TlsCertificateValidator::validate() -> X509CertificateValidation {
    if (_peerCertificates.isEmpty()) {
        return directRejection(
            X509CertificateValidationFailureCategory::EmptyPeerCertificates,
            {},
            "The peer supplied no target certificate."_el);
    }
    if (_policy.trustAnchors().isEmpty()) {
        return directRejection(
            X509CertificateValidationFailureCategory::EmptyTrustAnchors,
            {},
            "The policy contains no explicit trust anchor."_el);
    }
    if (!_validationTime.isValid()) {
        return directRejection(
            X509CertificateValidationFailureCategory::InvalidValidationTime,
            _peerCertificates.certificates().first(),
            "The requested validation time is invalid."_el);
    }

    // RFC 9525 section 6.2 and RFC 9549: validate the canonical A-label form before path work. IP references retain
    // their canonical network::IpAddress representation.
    if (!validateReferenceIdentity()) {
        return rejectedResult();
    }

    // RFC 4158 sections 2.4 and 5: create one deterministic issuer pool in peer, configured, then anchor order,
    // deduplicating by exact certificate DER and enforcing the aggregate work bound.
    if (!collectCandidates()) {
        return rejectedResult();
    }

    const auto target = _peerCertificates.certificates().first();
    _path.append(target);

    // RFC 5280 section 6.1: an explicitly trusted target terminates the path immediately, while this policy still
    // applies all end-entity checks to that same certificate.
    if (isExplicitAnchor(target)) {
        if (validateSelectedPath()) {
            return X509CertificateValidation{_path};
        }
        return rejectedResult();
    }

    // RFC 4158 section 5: build forward from the target with deterministic depth-first traversal and alternate-branch
    // backtracking. The path is retained in target-to-anchor order.
    if (auto accepted = buildPath(target); accepted.has_value()) {
        return std::move(accepted).value();
    }
    return rejectedResult();
}

auto X509TlsCertificateValidator::collectCandidates() -> bool {
    auto peerCount = std::size_t{1U};
    const auto targetDer = _peerCertificates.certificates().first().toDer();
    auto addPeer = [this, &peerCount, &targetDer](const X509Certificate &certificate) {
        if (certificate.toDer() != targetDer && !_candidateIndex.contains(certificate.toDer())) {
            if (++peerCount > X509TlsCertificatePolicy::cMaximumCandidateCertificates) {
                recordFailure(
                    X509CertificateValidationFailureCategory::CandidateLimitExceeded,
                    certificate,
                    "The peer and intermediate certificate limit of 256 was exceeded."_el);
                return false;
            }
            addCandidate(certificate, false);
        }
        return true;
    };
    for (const auto &certificate : _peerCertificates.certificates()) {
        if (!addPeer(certificate)) {
            return false;
        }
    }
    for (const auto &certificate : _policy.intermediates().certificates()) {
        if (!addPeer(certificate)) {
            return false;
        }
    }
    // Trust anchors have a separate bound and index; a normal distribution bundle never consumes peer capacity.
    if (_policy.trustAnchors().certificates().count().toSizeT() > X509TlsCertificatePolicy::cMaximumTrustAnchors) {
        recordFailure(
            X509CertificateValidationFailureCategory::CandidateLimitExceeded,
            {},
            "The trust-anchor limit of 4096 was exceeded."_el);
        return false;
    }
    for (const auto &certificate : _policy.trustAnchors().certificates()) {
        addCandidate(certificate, true);
    }
    addCandidate(_peerCertificates.certificates().first(), false);
    return true;
}

void X509TlsCertificateValidator::addCandidate(const X509Certificate &certificate, const bool isAnchor) {
    const auto certificateDer = certificate.toDer();
    if (const auto existing = _candidateIndex.find(certificateDer); existing != _candidateIndex.end()) {
        _candidates[existing->second].isAnchor |= isAnchor;
        return;
    }
    const auto index = _candidates.size();
    _candidateIndex.emplace(certificateDer, index);
    _issuerIndex[certificate.subject().asn1().encodedData()].push_back(index);
    _candidates.push_back(Candidate{certificate, isAnchor});
}

auto X509TlsCertificateValidator::buildPath(const X509Certificate &child) -> std::optional<X509CertificateValidation> {
    if (_resourceLimitExhausted) {
        return {};
    }
    auto foundNameCandidate = false;
    const auto issuers = _issuerIndex.find(child.issuer().asn1().encodedData());
    const auto emptyCandidates = std::vector<std::size_t>{};
    const auto &candidateIndexes = issuers == _issuerIndex.end() ? emptyCandidates : issuers->second;
    for (const auto candidateIndex : candidateIndexes) {
        const auto &candidate = _candidates[candidateIndex];
        // RFC 4158 section 2.4: an issuer candidate must link the child issuer to its subject. This initial portable
        // increment deliberately requires byte-identical canonical DER Names and uses AKI/SKI only as a mismatch
        // filter.
        if (!hasIssuerLink(child, candidate.certificate)) {
            continue;
        }
        foundNameCandidate = true;

        // RFC 4158 section 5.1: reject a certificate already present in the prospective path, preventing cycles while
        // leaving other deterministic issuer branches available.
        if (isInCurrentPath(candidate.certificate)) {
            recordFailure(
                X509CertificateValidationFailureCategory::PathLoop,
                child,
                "The prospective path repeats a certificate."_el,
                candidate.certificate);
            continue;
        }

        // RFC 4158 section 5.2: enforce a hard path bound before adding the next issuer certificate.
        if (_path.count().toSizeT() >= X509TlsCertificatePolicy::cMaximumPathDepth) {
            recordFailure(
                X509CertificateValidationFailureCategory::PathDepthExceeded,
                child,
                "The prospective path exceeds the 16-certificate depth limit."_el,
                candidate.certificate);
            continue;
        }

        // RFC 5280 section 6.1.3(a)(1): verify the exact child TBSCertificate and signature with the prospective
        // issuer public key before extending the path. Cached edges do not consume the verification-attempt budget.
        const auto edgeStatus = verifyEdge(child, candidate.certificate);
        if (edgeStatus != EdgeStatus::Valid) {
            if (_resourceLimitExhausted) {
                return {};
            }
            recordFailure(
                edgeStatus == EdgeStatus::Unsupported ? X509CertificateValidationFailureCategory::SignatureUnsupported
                                                      : X509CertificateValidationFailureCategory::SignatureInvalid,
                child,
                edgeStatus == EdgeStatus::Unsupported
                    ? "The certificate signature algorithm is unsupported or malformed."_el
                    : "The issuer candidate did not verify the certificate signature."_el,
                candidate.certificate);
            continue;
        }

        _path.append(candidate.certificate);
        if (candidate.isAnchor) {
            // RFC 5280 section 6.1.1: the configured anchor supplies trusted name and public-key input. Its
            // self-signature, validity, extensions, and CA constraints are intentionally not validated.
            if (validateSelectedPath()) {
                return X509CertificateValidation{_path};
            }
        } else if (auto accepted = buildPath(candidate.certificate); accepted.has_value()) {
            return accepted;
        }
        _path.removeLast();
        if (_resourceLimitExhausted) {
            return {};
        }
    }

    if (!foundNameCandidate) {
        recordFailure(
            X509CertificateValidationFailureCategory::IssuerNotFound,
            child,
            "No issuer candidate has the required exact subject-name linkage."_el);
    }
    return {};
}

auto X509TlsCertificateValidator::isExplicitAnchor(const X509Certificate &certificate) const -> bool {
    const auto der = certificate.toDer();
    for (const auto &anchor : _policy.trustAnchors().certificates()) {
        if (anchor.toDer() == der) {
            return true;
        }
    }
    return false;
}

auto X509TlsCertificateValidator::isInCurrentPath(const X509Certificate &certificate) const -> bool {
    const auto der = certificate.toDer();
    for (const auto &pathCertificate : _path) {
        if (pathCertificate.toDer() == der) {
            return true;
        }
    }
    return false;
}

auto X509TlsCertificateValidator::hasIssuerLink(const X509Certificate &child, const X509Certificate &issuer) -> bool {
    if (child.issuer().asn1().encodedData() != issuer.subject().asn1().encodedData()) {
        return false;
    }
    const auto authorityKeyIdentifier = child.issuerId();
    const auto subjectKeyIdentifier = issuer.subjectId();
    return authorityKeyIdentifier.isEmpty() || subjectKeyIdentifier.isEmpty() ||
        authorityKeyIdentifier == subjectKeyIdentifier;
}

auto X509TlsCertificateValidator::verifyEdge(const X509Certificate &child, const X509Certificate &issuer)
    -> EdgeStatus {
    const auto childDer = child.toDer();
    const auto issuerDer = issuer.toDer();
    for (const auto &edge : _verifiedEdges) {
        if (edge.childDer == childDer && edge.issuerDer == issuerDer) {
            return edge.status;
        }
    }

    if (_signatureVerificationCount >= X509TlsCertificatePolicy::cMaximumSignatureVerifications) {
        recordFailure(
            X509CertificateValidationFailureCategory::SignatureLimitExceeded,
            child,
            "The 1024-attempt signature-verification limit was exhausted."_el,
            issuer);
        _resourceLimitExhausted = true;
        return EdgeStatus::Unsupported;
    }
    ++_signatureVerificationCount;

    // RFC 5280 section 4.1.1.3: a certificate signature BIT STRING must have no unused signature bits for the
    // supported signature encodings.
    if (child.signatureUnusedBitCount() != 0U) {
        _verifiedEdges.push_back(VerifiedEdge{childDer, issuerDer, EdgeStatus::Invalid});
        return EdgeStatus::Invalid;
    }

    auto status = EdgeStatus::Unsupported;
    try {
        status = issuer.publicKey().verifySignature(
                     child.signatureAlgorithm(), child.tbsCertificateDer().span(), child.signatureData().span())
            ? EdgeStatus::Valid
            : EdgeStatus::Invalid;
    } catch (const err::RuntimeError &) {
        status = EdgeStatus::Unsupported;
    }
    _verifiedEdges.push_back(VerifiedEdge{childDer, issuerDer, status});
    return status;
}

auto X509TlsCertificateValidator::validateSelectedPath() -> bool {
    const auto pathCount = _path.count().toSizeT();
    const auto target = _path.first();

    // RFC 5280 section 6.1 and RFC 8446 section 4.4.2.2: validate the target even when it is itself the configured
    // anchor; direct trust does not bypass its profile, time, critical-extension, purpose, or identity checks.
    if (!validateNonAnchorCertificate(target) || !validateTarget(target)) {
        return false;
    }

    // RFC 5280 section 6.1: process every non-anchor intermediate from the target toward the trust anchor. The last
    // path element is trusted input and is deliberately excluded from these certificate checks.
    for (auto pathIndex = std::size_t{1U}; pathIndex + 1U < pathCount; ++pathIndex) {
        const auto certificate = _path.get(unit::ItemIndex::fromSizeT(pathIndex));
        if (!validateNonAnchorCertificate(certificate) || !validateIntermediate(certificate, pathIndex)) {
            return false;
        }
    }

    // RFC 9525 section 6: authenticate the requested service identity only after a valid certification path and TLS
    // server purpose have been established.
    if (!matchServerIdentity(target)) {
        if (_invalidPresentedIdentityDiagnostic.has_value()) {
            recordFailure(
                X509CertificateValidationFailureCategory::InvalidPresentedIdentity,
                target,
                _invalidPresentedIdentityDiagnostic.value());
        } else {
            recordFailure(
                X509CertificateValidationFailureCategory::ServerIdentityMismatch,
                target,
                "No subjectAltName matches the requested DNS name or IP address."_el);
        }
        return false;
    }
    return true;
}

}
