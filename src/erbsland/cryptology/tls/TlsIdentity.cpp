// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsIdentity.hpp"

#include "../CryptologyError.hpp"
#include "../impl/SoftwareTlsSigner.hpp"

#include "../../err/ParameterFailure.hpp"
#include "../../err/RuntimeError.hpp"
#include "../../text/Literals.hpp"
#include "../../unit/ItemCount.hpp"

#include <array>

namespace erbsland::cryptology {

using namespace err;
using namespace text::literals;

TlsIdentity::TlsIdentity(X509CertificateBundle certificateChain, SigningPrivateKey signingKey) :
    _certificateChain{std::move(certificateChain)} {
    if (_certificateChain.isEmpty() || signingKey.isEmpty()) {
        throw ParameterFailure{"A TLS identity requires a certificate chain and signing key."_el, "identity"_el};
    }
    // RFC 8446 section 4.4.2: validate matching before transferring ownership of the protected private key.
    if (!signingKey.matches(_certificateChain.certificates().first().publicKey())) {
        throw CryptologyError{"The TLS leaf certificate does not match the signing key."_el};
    }
    _signer = std::make_shared<impl::SoftwareTlsSigner>(std::move(signingKey));
    requireUsableSigner();
}

TlsIdentity::TlsIdentity(X509CertificateBundle certificateChain, impl::TlsSignerPtr signer) :
    _certificateChain{std::move(certificateChain)}, _signer{std::move(signer)} {
    if (_certificateChain.isEmpty() || _signer == nullptr) {
        throw ParameterFailure{"A TLS identity requires a certificate chain and signer."_el, "identity"_el};
    }
    requireUsableSigner();
}

void TlsIdentity::requireUsableSigner() const {
    // RFC 8446 section 4.4.2: bound configured client and server identities identically before constructing flights.
    if (_certificateChain.certificates().count() > unit::ItemCount{16U}) {
        throw ParameterFailure{"A TLS identity exceeds the 16-certificate bound."_el, "certificateChain"_el};
    }
    auto encodedLength = std::size_t{4U};
    for (const auto &certificate : _certificateChain.certificates()) {
        encodedLength += 5U + certificate.toDer().length().toSizeT();
    }
    if (encodedLength > 1024U * 1024U) {
        throw ParameterFailure{"The TLS Certificate message exceeds one MiB."_el, "certificateChain"_el};
    }
    // RFC 8446 section 4.2.3: require at least one permitted CertificateVerify scheme before accepting the identity.
    static constexpr auto cSchemes = std::array{
        TlsSignatureScheme{TlsSignatureScheme::Ed25519},
        TlsSignatureScheme{TlsSignatureScheme::EcdsaSecp256r1Sha256},
        TlsSignatureScheme{TlsSignatureScheme::RsaPssRsaeSha256},
        TlsSignatureScheme{TlsSignatureScheme::RsaPssRsaeSha384},
        TlsSignatureScheme{TlsSignatureScheme::RsaPssRsaeSha512},
        TlsSignatureScheme{TlsSignatureScheme::EcdsaSecp384r1Sha384},
        TlsSignatureScheme{TlsSignatureScheme::RsaPssPssSha256},
        TlsSignatureScheme{TlsSignatureScheme::RsaPssPssSha384},
        TlsSignatureScheme{TlsSignatureScheme::RsaPssPssSha512}};
    for (const auto scheme : cSchemes) {
        if (_signer->supports(scheme)) {
            return;
        }
    }
    throw CryptologyError{"The TLS identity supports no TLS 1.3 signature scheme."_el};
}

auto TlsIdentity::selectSignatureScheme(const util::List<TlsSignatureScheme> &offeredSchemes) const noexcept
    -> std::optional<TlsSignatureScheme> {
    // RFC 8446 section 4.2.3: preserve ClientHello preference and reject certificate-only PKCS#1 through supports().
    for (const auto scheme : offeredSchemes) {
        if (_signer->supports(scheme)) {
            return scheme;
        }
    }
    return std::nullopt;
}

auto TlsIdentity::isNative() const noexcept -> bool {
    return _signer->isNative();
}

auto TlsIdentity::signTlsCertificateVerify(const TlsSignatureScheme scheme, const mem::ConstByteSpan message) const
    -> mem::ByteBlock {
    if (!_signer->supports(scheme)) {
        throw CryptologyError{"The TLS identity does not support the selected signature scheme."_el};
    }
    auto signature = _signer->sign(scheme, message);
    // RFC 8446 section 4.4.3: verify provider output against the configured certificate, detecting wrong keys,
    // nonconforming PSS salt/hash parameters, malformed ECDSA encodings, and native-provider faults.
    auto verified = false;
    try {
        verified = _certificateChain.certificates().first().publicKey().verifyTlsCertificateVerifySignature(
            scheme, message, signature.span());
    } catch (const RuntimeError &) {
        throw CryptologyError{"The TLS identity provider returned a malformed signature."_el};
    }
    if (!verified) {
        throw CryptologyError{"The TLS identity provider returned an invalid signature."_el};
    }
    return signature;
}

}
