// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PublicKey.hpp"

#include "../impl/algorithm/ecdsa_signature/EcdsaSignature.hpp"
#include "../impl/algorithm/ed25519_signature/Ed25519Signature.hpp"
#include "../impl/algorithm/rsa_signature/RsaSignature.hpp"
#include "../impl/CryptologyOids.hpp"
#include "../impl/PemCodec.hpp"
#include "../impl/PemDerFileTools.hpp"
#include "../impl/X509Parser.hpp"
#include "../tls/TlsSignatureScheme.hpp"

#include "../../err/LogicError.hpp"
#include "../../err/ParseError.hpp"
#include "../../err/RuntimeError.hpp"
#include "../../path/Path.hpp"
#include "../../text/Literals.hpp"
#include "../../unit/ItemIndex.hpp"

namespace erbsland::cryptology {

using namespace mem;
using namespace text;
using namespace text::literals;
using namespace unit;

auto PublicKey::verifySignature(
    const X509AlgorithmIdentifier &signatureAlgorithm, const ConstByteSpan message, const ConstByteSpan signature) const
    -> bool {
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
    const TlsSignatureScheme scheme, const ConstByteSpan message, const ConstByteSpan signature) const -> bool {
    if (isEmpty()) {
        throw err::LogicError{"Cannot verify a signature with an empty public key."_el};
    }

    // RFC 8446 section 4.2.3 defines PKCS#1 v1.5 values only for certificate signatures, never CertificateVerify.
    if (!scheme.isAllowedForCertificateVerify()) {
        throw err::ParseError{"The TLS signature scheme is not allowed for CertificateVerify."_el};
    }

    // RFC 8446 section 4.2.3 binds RSA-PSS-RSAE schemes to rsaEncryption and RSA-PSS-PSS schemes to id-RSASSA-PSS.
    const auto publicKeyOid = algorithm().oid().toString();
    if (scheme.requiresRsaEncryptionKey() && publicKeyOid != impl::cryptology_oids::rsaEncryption) {
        throw err::ParseError{"The TLS RSA-PSS-RSAE scheme requires an rsaEncryption public key."_el};
    }
    if (scheme.requiresRsaPssKey() && publicKeyOid != impl::cryptology_oids::rsaPss) {
        throw err::ParseError{"The TLS RSA-PSS-PSS scheme requires an id-RSASSA-PSS public key."_el};
    }

    // RFC 8446 sections 4.2.3 and 4.4.3 apply the mapped scheme to the exact caller-constructed signed content.
    return verifySignature(scheme.signatureAlgorithmIdentifier(), message, signature);
}

auto PublicKey::toPem() const -> String {
    return isEmpty() ? String{} : impl::PemCodec{_der, impl::PemLabel::PublicKey}.encode();
}

void PublicKey::writeToFile(const path::Path &path, const PemDerFormat format) const {
    if (isEmpty()) {
        throw err::LogicError{"A public key is required for writing."_el};
    }
    const auto file = impl::PemDerFileTools{path, impl::PemDerFileTools::Artifact::PublicKey};
    if (file.outputFormat(format) == PemDerFormat::Pem) {
        file.writePem(toPem());
    } else {
        file.writeDer(_der);
    }
}

auto PublicKey::fromDer(const ByteBlock &der) noexcept -> PublicKey {
    try {
        return fromDerOrThrow(der);
    } catch (const err::RuntimeError &) {
        return {};
    }
}

auto PublicKey::fromDerOrThrow(const ByteBlock &der) -> PublicKey {
    return impl::X509Parser::parsePublicKey(der);
}

auto PublicKey::fromPem(const String &pem) noexcept -> PublicKey {
    try {
        return fromPemOrThrow(pem);
    } catch (const err::RuntimeError &) {
        return {};
    }
}

auto PublicKey::fromPemOrThrow(const String &pem) -> PublicKey {
    const auto values = impl::PemCodec{pem, impl::PemLabel::PublicKey}.decode();
    return fromDerOrThrow(values.getRefOrThrow(ItemIndex::zero()));
}

auto PublicKey::fromFile(const path::Path &path, const PemDerFormat format) noexcept -> PublicKey {
    try {
        return fromFileOrThrow(path, format);
    } catch (const err::RuntimeError &) {
        return {};
    }
}

auto PublicKey::fromFileOrThrow(const path::Path &path, const PemDerFormat format) -> PublicKey {
    const auto file = impl::PemDerFileTools{path, impl::PemDerFileTools::Artifact::PublicKey};
    const auto data = file.read();
    if (file.inputFormat(data, format) == PemDerFormat::Pem) {
        return fromPemOrThrow(impl::PemDerFileTools::toPemText(data));
    }
    return fromDerOrThrow(data);
}

}
