#include "TlsClientProtocol.hpp"

#include "../TlsProtocolError.hpp"
#include "../TlsWireWriter.hpp"

#include "../../../../cryptology/impl/algorithm/rsa_signature/RsaSignature.hpp"
#include "../../../../err/RuntimeError.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../unit/ByteCount.hpp"

namespace erbsland::network::impl {

using namespace text::literals;

void TlsClientProtocol::completeSignature(mem::ByteBlock signature) noexcept {
    if (_checkpoint != TlsClientProtocolCheckpoint::Signature || !_pendingSignature.has_value()) {
        return;
    }
    try {
        if (signature.isEmpty() || signature.length() > unit::ByteCount{65535U}) {
            throw TlsProtocolError{
                TlsAlertDescription::InternalError, "A TLS signer returned an invalid signature size."_el};
        }
        _checkpoint = TlsClientProtocolCheckpoint::None;
        finishSignedFlight(signature);
        _pendingSignature.reset();
        processRetainedInputNoThrow();
    } catch (const TlsProtocolError &error) {
        fail(error.alert(), error.reason());
    } catch (const err::RuntimeError &error) {
        fail(TlsAlertDescription::InternalError, error.reason());
    }
}

void TlsClientProtocol::failPendingOperation(text::String diagnostic) noexcept {
    if (_state == TlsClientProtocolState::Handshaking) {
        fail(TlsAlertDescription::InternalError, std::move(diagnostic));
    }
}

void TlsClientProtocol::finishSignedFlight(const mem::ByteBlock &signature) {
    // RFC 8446 section 4.4.3: CertificateVerify covers the transcript through the client Certificate.
    auto body = TlsWireWriter{};
    body.writeU16(_clientSignatureScheme->toRawValue());
    body.writeVector16(signature.span());
    const auto bodyBytes = body.finish();
    auto message = TlsWireWriter{};
    message.writeU8(15U);
    message.writeU24(static_cast<uint32_t>(bodyBytes.length().toSizeT()));
    message.writeBytes(bodyBytes.span());
    const auto encoded = message.finish();
    queueProtectedHandshake(encoded.span());
    _transcript->update(encoded.span());
    finishClientFlight();
}

auto TlsClientProtocol::selectClientIdentity() const -> std::optional<cryptology::TlsSignatureScheme> {
    const auto &identity = _options.clientIdentity();
    if (identity == nullptr || _hasUnsupportedOidFilters) {
        return std::nullopt;
    }
    const auto &chain = identity->certificateChain().certificates();
    const auto &leaf = chain.first();
    // RFC 8446 sections 4.4.2.2 and 4.2.4: satisfy purpose, validity and requested authorities before disclosure.
    if (_options.validationTime() < leaf.validFrom() || _options.validationTime() > leaf.validTo()) {
        return std::nullopt;
    }
    if (const auto usage = leaf.keyUsage();
        usage.has_value() && !usage->isSet(cryptology::X509KeyUsage::DigitalSignature)) {
        return std::nullopt;
    }
    const auto usages = leaf.extendedKeyUsage();
    auto permitsClient = usages.isEmpty();
    for (const auto &usage : usages) {
        if (usage.toString() == "1.3.6.1.5.5.7.3.2"_el || usage.toString() == "2.5.29.37.0"_el) {
            permitsClient = true;
        }
    }
    if (!permitsClient) {
        return std::nullopt;
    }
    auto authorityMatches = _requestedAuthorities.empty();
    for (const auto &certificate : chain) {
        for (const auto &authority : _requestedAuthorities) {
            if (certificate.issuer().asn1().encodedData() == authority) {
                authorityMatches = true;
            }
        }
    }
    if (!authorityMatches) {
        return std::nullopt;
    }
    // RFC 8446 section 4.2.3: all non-self-signed certificate signatures must satisfy the separate certificate
    // list (or its signature_algorithms fallback). Unknown constraints never widen the offered permission.
    for (const auto &certificate : chain) {
        if (certificate.subjectId() == certificate.issuerId()) {
            try {
                if (certificate.publicKey().verifySignature(
                        certificate.signatureAlgorithm(),
                        certificate.tbsCertificateDer().span(),
                        certificate.signatureData().span())) {
                    continue;
                }
            } catch (const err::RuntimeError &) {
                return std::nullopt;
            }
        }
        auto supported = false;
        for (const auto scheme : _requestedCertificateSchemes) {
            const auto algorithm = scheme.signatureAlgorithmIdentifier();
            if (algorithm.oid() == certificate.signatureAlgorithm().oid()) {
                if (scheme.toRawValue() < 0x0804U || scheme == cryptology::TlsSignatureScheme::Ed25519) {
                    supported = true;
                } else {
                    try {
                        const auto expected = cryptology::impl::rsa_signature::decodeSignatureParameters(algorithm);
                        const auto actual = cryptology::impl::rsa_signature::decodeSignatureParameters(
                            certificate.signatureAlgorithm());
                        supported = expected.padding == actual.padding && expected.hash == actual.hash &&
                            expected.saltLength == actual.saltLength;
                    } catch (const err::RuntimeError &) {
                        return std::nullopt;
                    }
                }
            }
        }
        if (!supported) {
            return std::nullopt;
        }
    }
    return identity->selectSignatureScheme(_requestedSignatureSchemes);
}

}
