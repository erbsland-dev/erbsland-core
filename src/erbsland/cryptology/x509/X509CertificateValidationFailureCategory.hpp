// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cryptology {

/// A stable failure category for X.509 server-certificate validation.
enum class X509CertificateValidationFailureCategory : uint8_t {
    EmptyPeerCertificates,      ///< The peer supplied no target certificate.
    EmptyTrustAnchors,          ///< The policy contains no explicit trust anchor.
    InvalidValidationTime,      ///< The requested validation time is invalid.
    InvalidReferenceIdentity,   ///< The reference identity cannot be canonicalized for comparison.
    InvalidPresentedIdentity,   ///< A relevant presented dNSName is malformed or violates IDNA2008.
    CandidateLimitExceeded,     ///< The aggregate unique-certificate limit was exceeded.
    PathDepthExceeded,          ///< Every prospective path exceeded the depth limit.
    SignatureLimitExceeded,     ///< The signature-verification work limit was exceeded.
    PathLoop,                   ///< Every prospective path repeated a certificate.
    IssuerNotFound,             ///< No matching issuer or trust anchor could be found.
    SignatureInvalid,           ///< An issuer candidate did not verify the child signature.
    SignatureUnsupported,       ///< A signature or public-key construction is unsupported or malformed.
    CertificateProfileRejected, ///< Compatible-parser profile issues are not accepted by this policy.
    CertificateNotYetValid,     ///< The validation time precedes notBefore.
    CertificateExpired,         ///< The validation time follows notAfter.
    UnknownCriticalExtension,   ///< A non-anchor certificate has an unsupported critical extension.
    BasicConstraintsRequired,   ///< An intermediate has no critical Basic Constraints extension.
    NotCertificateAuthority,    ///< An intermediate is not a CA, or the target is marked as one.
    KeyUsageRejected,           ///< Key Usage does not permit the required operation.
    ExtendedKeyUsageRejected,   ///< Extended Key Usage does not permit TLS server authentication.
    PathLengthExceeded,         ///< A CA pathLenConstraint is exceeded.
    ServerIdentityMismatch,     ///< No appropriate subjectAltName matches the requested identity.
};

}
