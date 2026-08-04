// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "X509ServerCertificateValidator.hpp"

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

auto X509ServerCertificateValidator::validate() -> X509CertificateValidation {
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

auto X509ServerCertificateValidator::collectCandidates() -> bool {
    auto uniqueCertificateCount = std::size_t{1U}; // The target is part of the aggregate bound.
    const auto targetDer = _peerCertificates.certificates().first().toDer();
    auto isNew = [&targetDer, this](const X509Certificate &certificate) {
        if (certificate.toDer() == targetDer) {
            return false;
        }
        for (const auto &candidate : _candidates) {
            if (candidate.certificate.toDer() == certificate.toDer()) {
                return false;
            }
        }
        return true;
    };
    auto addBounded = [&uniqueCertificateCount, &isNew, this](const X509Certificate &certificate, bool anchor) {
        if (isNew(certificate)) {
            ++uniqueCertificateCount;
            if (uniqueCertificateCount > X509ServerCertificatePolicy::cMaximumCandidateCertificates) {
                recordFailure(
                    X509CertificateValidationFailureCategory::CandidateLimitExceeded,
                    _peerCertificates.certificates().first(),
                    "The aggregate unique-certificate limit of 256 was exceeded."_el);
                _resourceLimitExhausted = true;
                return false;
            }
        }
        addCandidate(certificate, anchor);
        return true;
    };

    auto first = true;
    for (const auto &certificate : _peerCertificates.certificates()) {
        if (first) {
            first = false;
            continue;
        }
        if (!addBounded(certificate, false)) {
            return false;
        }
    }
    for (const auto &certificate : _policy.intermediates().certificates()) {
        if (!addBounded(certificate, false)) {
            return false;
        }
    }
    for (const auto &certificate : _policy.trustAnchors().certificates()) {
        if (!addBounded(certificate, true)) {
            return false;
        }
    }

    // RFC 4158 section 5.1: retain the target as a final, non-issuer fallback solely so an untrusted self-issued or
    // cyclic path is reported as certificate repetition. It is already included in the aggregate unique count.
    _candidates.push_back(Candidate{_peerCertificates.certificates().first(), false});
    return true;
}

void X509ServerCertificateValidator::addCandidate(const X509Certificate &certificate, const bool isAnchor) {
    const auto certificateDer = certificate.toDer();
    for (auto &candidate : _candidates) {
        if (candidate.certificate.toDer() == certificateDer) {
            candidate.isAnchor = candidate.isAnchor || isAnchor;
            return;
        }
    }
    if (certificateDer == _peerCertificates.certificates().first().toDer()) {
        return;
    }
    _candidates.push_back(Candidate{certificate, isAnchor});
}

auto X509ServerCertificateValidator::buildPath(const X509Certificate &child)
    -> std::optional<X509CertificateValidation> {
    if (_resourceLimitExhausted) {
        return {};
    }
    auto foundNameCandidate = false;
    for (const auto &candidate : _candidates) {
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
        if (_path.count().toSizeT() >= X509ServerCertificatePolicy::cMaximumPathDepth) {
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

auto X509ServerCertificateValidator::isExplicitAnchor(const X509Certificate &certificate) const -> bool {
    const auto der = certificate.toDer();
    for (const auto &anchor : _policy.trustAnchors().certificates()) {
        if (anchor.toDer() == der) {
            return true;
        }
    }
    return false;
}

auto X509ServerCertificateValidator::isInCurrentPath(const X509Certificate &certificate) const -> bool {
    const auto der = certificate.toDer();
    for (const auto &pathCertificate : _path) {
        if (pathCertificate.toDer() == der) {
            return true;
        }
    }
    return false;
}

auto X509ServerCertificateValidator::hasIssuerLink(const X509Certificate &child, const X509Certificate &issuer)
    -> bool {
    if (child.issuer().asn1().encodedData() != issuer.subject().asn1().encodedData()) {
        return false;
    }
    const auto authorityKeyIdentifier = child.issuerId();
    const auto subjectKeyIdentifier = issuer.subjectId();
    return authorityKeyIdentifier.isEmpty() || subjectKeyIdentifier.isEmpty() ||
        authorityKeyIdentifier == subjectKeyIdentifier;
}

auto X509ServerCertificateValidator::verifyEdge(const X509Certificate &child, const X509Certificate &issuer)
    -> EdgeStatus {
    const auto childDer = child.toDer();
    const auto issuerDer = issuer.toDer();
    for (const auto &edge : _verifiedEdges) {
        if (edge.childDer == childDer && edge.issuerDer == issuerDer) {
            return edge.status;
        }
    }

    if (_signatureVerificationCount >= X509ServerCertificatePolicy::cMaximumSignatureVerifications) {
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
    } catch (const std::exception &) {
        status = EdgeStatus::Unsupported;
    }
    _verifiedEdges.push_back(VerifiedEdge{childDer, issuerDer, status});
    return status;
}

auto X509ServerCertificateValidator::validateSelectedPath() -> bool {
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

auto X509ServerCertificateValidator::validateNonAnchorCertificate(const X509Certificate &certificate) -> bool {
    // RFC 5280 section 6.1.3: compatible-parser profile deviations remain explicit and are not accepted for any
    // non-anchor certificate.
    if (!certificate.profileIssues().isEmpty()) {
        recordFailure(
            X509CertificateValidationFailureCategory::CertificateProfileRejected,
            certificate,
            "The certificate contains compatible-parser profile issues."_el);
        return false;
    }

    // RFC 5280 sections 4.1.2.5 and 6.1.3(a)(2): validity is inclusive at both notBefore and notAfter.
    if (_validationTime < certificate.validFrom()) {
        recordFailure(
            X509CertificateValidationFailureCategory::CertificateNotYetValid,
            certificate,
            "The certificate is not yet valid at the requested validation time."_el);
        return false;
    }
    if (_validationTime > certificate.validTo()) {
        recordFailure(
            X509CertificateValidationFailureCategory::CertificateExpired,
            certificate,
            "The certificate is expired at the requested validation time."_el);
        return false;
    }

    // RFC 5280 section 4.2: every unrecognized critical extension prevents certificate use. The portable parser
    // currently recognizes SKI, AKI, SAN, Basic Constraints, Key Usage, and Extended Key Usage.
    return validateCriticalExtensions(certificate);
}

auto X509ServerCertificateValidator::validateCriticalExtensions(const X509Certificate &certificate) -> bool {
    for (const auto &extension : certificate.extensions()) {
        if (!extension.isCritical()) {
            continue;
        }
        const auto oid = extension.oid().toString();
        if (oid == "2.5.29.14"_el || oid == "2.5.29.15"_el || oid == "2.5.29.17"_el || oid == "2.5.29.19"_el ||
            oid == "2.5.29.35"_el || oid == "2.5.29.37"_el) {
            continue;
        }
        recordFailure(
            X509CertificateValidationFailureCategory::UnknownCriticalExtension,
            certificate,
            "The certificate contains an unsupported critical extension."_el);
        return false;
    }
    return true;
}

auto X509ServerCertificateValidator::validateTarget(const X509Certificate &certificate) -> bool {
    // RFC 5280 section 4.2.1.9: a TLS server end entity must not assert Basic Constraints cA=true.
    if (const auto constraints = certificate.basicConstraints();
        constraints.has_value() && constraints->isCertificateAuthority()) {
        recordFailure(
            X509CertificateValidationFailureCategory::NotCertificateAuthority,
            certificate,
            "The target certificate asserts Basic Constraints cA=true."_el);
        return false;
    }

    // RFC 5280 section 4.2.1.3 and RFC 8446 section 4.4.2.2: when Key Usage is present, the server certificate must
    // permit digital signatures used by TLS CertificateVerify.
    if (const auto keyUsage = certificate.keyUsage();
        keyUsage.has_value() && !keyUsage->isSet(X509KeyUsage::DigitalSignature)) {
        recordFailure(
            X509CertificateValidationFailureCategory::KeyUsageRejected,
            certificate,
            "The target Key Usage does not permit digitalSignature."_el);
        return false;
    }

    // RFC 5280 section 4.2.1.12 and RFC 8446 section 4.4.2.2: an Extended Key Usage restriction must include
    // id-kp-serverAuth or anyExtendedKeyUsage.
    if (!permitsServerAuthentication(certificate)) {
        recordFailure(
            X509CertificateValidationFailureCategory::ExtendedKeyUsageRejected,
            certificate,
            "The target Extended Key Usage does not permit TLS server authentication."_el);
        return false;
    }
    return true;
}

auto X509ServerCertificateValidator::validateIntermediate(
    const X509Certificate &certificate, const std::size_t pathIndex) -> bool {
    // RFC 5280 sections 4.2.1.9 and 6.1.4: each non-anchor intermediate must contain critical Basic Constraints with
    // cA=true.
    const auto constraints = certificate.basicConstraints();
    if (!constraints.has_value() || !hasCriticalBasicConstraints(certificate)) {
        recordFailure(
            X509CertificateValidationFailureCategory::BasicConstraintsRequired,
            certificate,
            "An intermediate requires a critical Basic Constraints extension."_el);
        return false;
    }
    if (!constraints->isCertificateAuthority()) {
        recordFailure(
            X509CertificateValidationFailureCategory::NotCertificateAuthority,
            certificate,
            "An intermediate certificate does not assert cA=true."_el);
        return false;
    }

    // RFC 5280 sections 4.2.1.3 and 6.1.4: if Key Usage exists on an intermediate, keyCertSign must be asserted.
    if (const auto keyUsage = certificate.keyUsage();
        keyUsage.has_value() && !keyUsage->isSet(X509KeyUsage::KeyCertificateSign)) {
        recordFailure(
            X509CertificateValidationFailureCategory::KeyUsageRejected,
            certificate,
            "The intermediate Key Usage does not permit keyCertSign."_el);
        return false;
    }

    // RFC 5280 section 4.2.1.12: an intermediate EKU restriction is inherited by the path and therefore must permit
    // serverAuth or anyExtendedKeyUsage for this server-authentication policy.
    if (!permitsServerAuthentication(certificate)) {
        recordFailure(
            X509CertificateValidationFailureCategory::ExtendedKeyUsageRejected,
            certificate,
            "The intermediate Extended Key Usage does not permit TLS server authentication."_el);
        return false;
    }

    // RFC 5280 section 6.1.4(i): pathLenConstraint counts only non-self-issued intermediate CA certificates below the
    // current CA; the target certificate is not counted.
    if (constraints->pathLength().has_value() &&
        subordinateCaCount(pathIndex) > static_cast<std::size_t>(constraints->pathLength().value())) {
        recordFailure(
            X509CertificateValidationFailureCategory::PathLengthExceeded,
            certificate,
            "The intermediate Basic Constraints pathLenConstraint is exceeded."_el);
        return false;
    }
    return true;
}

auto X509ServerCertificateValidator::hasCriticalBasicConstraints(const X509Certificate &certificate) -> bool {
    for (const auto &extension : certificate.extensions()) {
        if (extension.oid().toString() == "2.5.29.19"_el) {
            return extension.isCritical();
        }
    }
    return false;
}

auto X509ServerCertificateValidator::permitsServerAuthentication(const X509Certificate &certificate) -> bool {
    const auto usages = certificate.extendedKeyUsage();
    if (usages.isEmpty()) {
        // RFC 5280 §4.2.1.12: An absent Extended Key Usage extension permits every purpose, but an explicitly
        // present empty sequence grants no purpose. Preserve that distinction because extendedKeyUsage() is empty
        // in both representations.
        for (const auto &extension : certificate.extensions()) {
            if (extension.oid().toString() == "2.5.29.37"_el) {
                return false;
            }
        }
        return true;
    }
    for (const auto &usage : usages) {
        const auto oid = usage.toString();
        if (oid == "1.3.6.1.5.5.7.3.1"_el || oid == "2.5.29.37.0"_el) {
            return true;
        }
    }
    return false;
}

auto X509ServerCertificateValidator::subordinateCaCount(const std::size_t pathIndex) const -> std::size_t {
    auto result = std::size_t{};
    for (auto index = std::size_t{1U}; index < pathIndex; ++index) {
        const auto certificate = _path.get(unit::ItemIndex::fromSizeT(index));
        if (!isSelfIssued(certificate)) {
            ++result;
        }
    }
    return result;
}

auto X509ServerCertificateValidator::isSelfIssued(const X509Certificate &certificate) -> bool {
    return certificate.issuer().asn1().encodedData() == certificate.subject().asn1().encodedData();
}

auto X509ServerCertificateValidator::validateReferenceIdentity() -> bool {
    if (_referenceIdentity.isAddress()) {
        return true;
    }
    try {
        const auto reference =
            parseDnsNameOrThrow(_referenceIdentity.name()->toString(network::HostNameFormat::IdnaAscii), false);
        return !reference.labels.empty();
    } catch (const err::ParseError &error) {
        recordFailure(
            X509CertificateValidationFailureCategory::InvalidReferenceIdentity,
            _peerCertificates.certificates().first(),
            error.reason());
        return false;
    }
}

auto X509ServerCertificateValidator::matchServerIdentity(const X509Certificate &certificate) -> bool {
    _invalidPresentedIdentityDiagnostic.reset();
    if (_referenceIdentity.isAddress()) {
        // RFC 9525 sections 6.1 and 6.4: IP references match only iPAddress subjectAltName values, comparing the
        // canonical address octets through network::IpAddress. Common Name and dNSName are never fallback identities.
        const auto referenceAddress = _referenceIdentity.address().value();
        for (const auto &presentedAddress : certificate.ipAddresses()) {
            if (presentedAddress == referenceAddress) {
                return true;
            }
        }
        return false;
    }

    const auto referenceName =
        parseDnsNameOrThrow(_referenceIdentity.name()->toString(network::HostNameFormat::IdnaAscii), false);
    // RFC 9525 sections 6.3 and 6.4: DNS references match only dNSName subjectAltName values. Presented wildcards are
    // limited to one complete leftmost label and consume exactly one reference label; Common Name is never inspected.
    for (const auto &presentedText : certificate.dnsNames()) {
        try {
            const auto presentedName = parseDnsNameOrThrow(presentedText, true);
            if (dnsNameMatches(referenceName, presentedName)) {
                return true;
            }
        } catch (const err::ParseError &error) {
            if (!_invalidPresentedIdentityDiagnostic.has_value()) {
                _invalidPresentedIdentityDiagnostic = error.reason();
            }
        }
    }
    return false;
}

auto X509ServerCertificateValidator::parseDnsNameOrThrow(const text::String &name, const bool allowWildcard)
    -> DnsName {
    auto source = name;
    auto result = DnsName{};
    if (name.startsWith("*."_el)) {
        if (!allowWildcard) {
            throw err::ParseError{"A DNS reference identity must not contain a wildcard."};
        }
        result.hasWildcard = true;
        result.labels.emplace_back("*"_el);
        source = name.slice(unit::ByteRange{unit::ByteIndex{2U}, name.length() - unit::ByteLength{2U}});
    } else if (name.contains("*"_el)) {
        throw err::ParseError{"A certificate wildcard must be the complete leftmost DNS label."};
    }
    const auto options = text::punycode::PunycodeOptions::network();
    const auto unicode = text::punycode::PunycodeDecoder{source, options}.decodeOrThrow();
    const auto canonical = text::punycode::PunycodeEncoder{unicode, options}.encodeOrThrow();
    auto label = text::StringEditor{};
    auto reader = text::StringCharReader{canonical};
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        if (character == U'.') {
            result.labels.emplace_back(label);
            label.clear();
            continue;
        }
        label.append(character);
    }
    result.labels.emplace_back(label);
    return result;
}

auto X509ServerCertificateValidator::dnsNameMatches(const DnsName &reference, const DnsName &presented) -> bool {
    if (reference.labels.size() != presented.labels.size()) {
        return false;
    }
    for (auto index = std::size_t{}; index < reference.labels.size(); ++index) {
        if (presented.hasWildcard && index == 0U) {
            continue;
        }
        if (reference.labels[index].compare(presented.labels[index], text::Char::compareAsciiFolded) !=
            std::strong_ordering::equal) {
            return false;
        }
    }
    return true;
}

void X509ServerCertificateValidator::recordFailure(
    const X509CertificateValidationFailureCategory category,
    std::optional<X509Certificate> certificate,
    const text::String &diagnostic,
    std::optional<X509Certificate> issuerCandidate) {
    const auto depth = _path.count().toSizeT();
    if (_bestFailure.has_value() && depth < _bestFailureDepth) {
        return;
    }
    _bestFailureDepth = depth;
    _bestFailure =
        X509CertificateValidationFailure{category, certificate, std::move(issuerCandidate), _path, diagnostic};
}

auto X509ServerCertificateValidator::rejectedResult() const -> X509CertificateValidation {
    if (_bestFailure.has_value()) {
        return X509CertificateValidation{_bestFailure.value()};
    }
    return directRejection(
        X509CertificateValidationFailureCategory::IssuerNotFound,
        _peerCertificates.certificates().first(),
        "No path to an explicit trust anchor could be constructed."_el);
}

auto X509ServerCertificateValidator::directRejection(
    const X509CertificateValidationFailureCategory category,
    std::optional<X509Certificate> certificate,
    const text::String &diagnostic) const -> X509CertificateValidation {
    return X509CertificateValidation{X509CertificateValidationFailure{category, certificate, {}, _path, diagnostic}};
}

}
