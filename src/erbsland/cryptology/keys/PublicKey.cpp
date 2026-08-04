// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PublicKey.hpp"

#include "../impl/algorithm/ecdsa_signature/EcdsaSignature.hpp"
#include "../impl/algorithm/ed25519_signature/Ed25519Signature.hpp"
#include "../impl/algorithm/rsa_signature/RsaSignature.hpp"
#include "../impl/X509Parser.hpp"
#include "../tls/TlsSignatureScheme.hpp"

#include "../../err/LogicError.hpp"
#include "../../err/ParseError.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::cryptology {

using namespace text::literals;

auto PublicKey::verifySignature(
    const X509AlgorithmIdentifier &signatureAlgorithm,
    const mem::ConstByteSpan message,
    const mem::ConstByteSpan signature) const -> bool {
    if (isEmpty()) {
        throw err::LogicError{"Cannot verify a signature with an empty public key."_el};
    }
    if (impl::ed25519_signature::isSignatureAlgorithm(signatureAlgorithm)) {
        return impl::ed25519_signature::verify(*this, signatureAlgorithm, message, signature);
    }
    if (impl::ecdsa_signature::isSignatureAlgorithm(signatureAlgorithm)) {
        return impl::ecdsa_signature::verify(*this, signatureAlgorithm, message, signature);
    }
    return impl::rsa_signature::verify(*this, signatureAlgorithm, message, signature);
}

auto PublicKey::verifyTlsCertificateVerifySignature(
    const TlsSignatureScheme scheme, const mem::ConstByteSpan message, const mem::ConstByteSpan signature) const
    -> bool {
    if (isEmpty()) {
        throw err::LogicError{"Cannot verify a signature with an empty public key."_el};
    }

    // RFC 8446 section 4.2.3 defines PKCS#1 v1.5 values only for certificate signatures, never CertificateVerify.
    if (!scheme.isAllowedForCertificateVerify()) {
        throw err::ParseError{"The TLS signature scheme is not allowed for CertificateVerify."_el};
    }

    // RFC 8446 section 4.2.3 binds RSA-PSS-RSAE schemes to rsaEncryption and RSA-PSS-PSS schemes to id-RSASSA-PSS.
    const auto publicKeyOid = algorithm().oid().toString();
    if (scheme.requiresRsaEncryptionKey() && publicKeyOid != "1.2.840.113549.1.1.1"_el) {
        throw err::ParseError{"The TLS RSA-PSS-RSAE scheme requires an rsaEncryption public key."_el};
    }
    if (scheme.requiresRsaPssKey() && publicKeyOid != "1.2.840.113549.1.1.10"_el) {
        throw err::ParseError{"The TLS RSA-PSS-PSS scheme requires an id-RSASSA-PSS public key."_el};
    }

    // RFC 8446 sections 4.2.3 and 4.4.3 apply the mapped scheme to the exact caller-constructed signed content.
    return verifySignature(scheme.signatureAlgorithmIdentifier(), message, signature);
}

auto PublicKey::fromDer(const mem::ByteBlock &der) noexcept -> PublicKey {
    try {
        return fromDerOrThrow(der);
    } catch (...) {
        return {};
    }
}

auto PublicKey::fromDerOrThrow(const mem::ByteBlock &der) -> PublicKey {
    return impl::X509Parser::parsePublicKey(der);
}

}
