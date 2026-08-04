// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsServerIdentity.hpp"

#include "../../err/ParameterError.hpp"
#include "../../text/Literals.hpp"

#include <array>

namespace erbsland::cryptology {

using namespace text::literals;

TlsServerIdentity::TlsServerIdentity(X509CertificateBundle certificateChain, SigningPrivateKey signingKey) :
    _certificateChain{std::move(certificateChain)}, _signingKey{std::move(signingKey)} {
    if (_certificateChain.isEmpty()) {
        throw err::ParameterError{
            "A TLS server identity requires a nonempty certificate chain."_el, "certificateChain"_el};
    }
    if (_signingKey.isEmpty()) {
        throw err::ParameterError{"A TLS server identity requires a private signing key."_el, "signingKey"_el};
    }

    // RFC 8446 section 4.4.2: the first CertificateEntry is the end-entity certificate authenticated by the key.
    if (!_signingKey.matches(_certificateChain.certificates().first().publicKey())) {
        throw err::ParameterError{
            "The TLS server leaf certificate does not match the signing key."_el, "signingKey"_el};
    }

    // RFC 8446 section 4.2.3: require at least one permitted CertificateVerify scheme before accepting the identity.
    static constexpr auto cSchemes = std::array{
        TlsSignatureScheme{TlsSignatureScheme::Ed25519},
        TlsSignatureScheme{TlsSignatureScheme::EcdsaSecp256r1Sha256},
        TlsSignatureScheme{TlsSignatureScheme::RsaPssRsaeSha256},
        TlsSignatureScheme{TlsSignatureScheme::RsaPssRsaeSha384},
        TlsSignatureScheme{TlsSignatureScheme::RsaPssPssSha256},
        TlsSignatureScheme{TlsSignatureScheme::RsaPssPssSha384}};
    for (const auto scheme : cSchemes) {
        if (_signingKey.supports(scheme)) {
            return;
        }
    }
    throw err::ParameterError{"The TLS server identity supports no TLS 1.3 signature scheme."_el, "signingKey"_el};
}

auto TlsServerIdentity::selectSignatureScheme(const util::List<TlsSignatureScheme> &offeredSchemes) const noexcept
    -> std::optional<TlsSignatureScheme> {
    // RFC 8446 section 4.2.3: preserve ClientHello preference and reject certificate-only PKCS#1 through supports().
    for (const auto scheme : offeredSchemes) {
        if (_signingKey.supports(scheme)) {
            return scheme;
        }
    }
    return std::nullopt;
}

}
