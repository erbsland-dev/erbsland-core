// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsServerProtocol.hpp"

#include "../TlsProtocolError.hpp"
#include "../TlsWireReader.hpp"
#include "../TlsWireWriter.hpp"

#include "../../../../cryptology/keys/PublicKey.hpp"
#include "../../../../err/RuntimeError.hpp"
#include "../../../../mem/ByteBlockEditor.hpp"
#include "../../../../text/Literals.hpp"

#include <algorithm>
#include <string_view>

namespace erbsland::network::impl {

using namespace mem;
using namespace text::literals;

void TlsServerProtocol::appendCertificateRequest(OutputFlight &flight) {
    if (_selectedConfiguration.clientAuthentication == cryptology::TlsClientAuthentication::Disabled) {
        return;
    }
    using Scheme = cryptology::TlsSignatureScheme;
    _clientSignatureSchemes = {
        Scheme::EcdsaSecp256r1Sha256,
        Scheme::EcdsaSecp384r1Sha384,
        Scheme::Ed25519,
        Scheme::RsaPssRsaeSha256,
        Scheme::RsaPssRsaeSha384,
        Scheme::RsaPssRsaeSha512,
        Scheme::RsaPssPssSha256,
        Scheme::RsaPssPssSha384,
        Scheme::RsaPssPssSha512};
    auto schemes = TlsWireWriter{};
    for (const auto scheme : _clientSignatureSchemes) {
        schemes.writeU16(scheme.toRawValue());
    }
    const auto schemeBytes = schemes.finish();
    auto signatureAlgorithms = TlsWireWriter{};
    signatureAlgorithms.writeVector16(schemeBytes.span());
    const auto signatureBytes = signatureAlgorithms.finish();
    auto extensions = TlsWireWriter{};
    extensions.writeU16(13U);
    extensions.writeVector16(signatureBytes.span());
    auto certificateSchemes = TlsWireWriter{};
    certificateSchemes.writeBytes(schemeBytes.span());
    for (const auto value : {Scheme::RsaPkcs1Sha256, Scheme::RsaPkcs1Sha384, Scheme::RsaPkcs1Sha512}) {
        certificateSchemes.writeU16(static_cast<uint16_t>(value));
    }
    const auto certificateSchemeBytes = certificateSchemes.finish();
    auto certificateAlgorithms = TlsWireWriter{};
    certificateAlgorithms.writeVector16(certificateSchemeBytes.span());
    const auto certificateAlgorithmBytes = certificateAlgorithms.finish();
    extensions.writeU16(50U);
    extensions.writeVector16(certificateAlgorithmBytes.span());
    const auto extensionBytes = extensions.finish();
    auto body = TlsWireWriter{};
    body.writeVector8({});
    body.writeVector16(extensionBytes.span());
    const auto bodyBytes = body.finish();
    auto writer = TlsWireWriter{};
    writer.writeU8(13U);
    writer.writeU24(static_cast<uint32_t>(bodyBytes.length().toSizeT()));
    writer.writeBytes(bodyBytes.span());
    const auto message = writer.finish();
    _transcript->update(message.span());
    appendProtectedHandshake(flight, message.span());
}

void TlsServerProtocol::processClientCertificate(const ByteBlock &message) {
    auto reader = TlsWireReader{message.span().subspan(4U)};

    // RFC 8446 section 4.4.2: the client Certificate in the main handshake uses the empty request context.
    if (!reader.readVector8().empty()) {
        throw TlsProtocolError{
            TlsAlertDescription::IllegalParameter, "The client Certificate request context must be empty."_el};
    }
    const auto certificateList = reader.readVector24();
    reader.requireEnd();
    if (certificateList.empty()) {
        if (_selectedConfiguration.clientAuthentication == cryptology::TlsClientAuthentication::Required) {
            throw TlsProtocolError{TlsAlertDescription::CertificateRequired, "A client certificate is required."_el};
        }
        _transcript->update(message.span());
        _handshakeStep = HandshakeStep::ClientFinished;
        return;
    }
    if (certificateList.size() > 1024U * 1024U) {
        throw TlsProtocolError{TlsAlertDescription::BadCertificate, "The client Certificate list exceeds one MiB."_el};
    }

    auto listReader = TlsWireReader{certificateList};
    auto certificates = std::vector<cryptology::X509Certificate>{};
    while (!listReader.isAtEnd()) {
        if (certificates.size() >= 16U) {
            throw TlsProtocolError{
                TlsAlertDescription::BadCertificate, "The client Certificate list exceeds 16 entries."_el};
        }
        const auto certificateData = listReader.readVector24();
        if (certificateData.empty()) {
            throw TlsProtocolError{TlsAlertDescription::BadCertificate, "A TLS Certificate entry is empty."_el};
        }
        const auto certificateExtensions = listReader.readVector16();
        forEachExtension(certificateExtensions, [&](const ExtensionView &) -> void {
            // No certificate-entry extension was requested by this server, so any response is unsolicited.
            throw TlsProtocolError{
                TlsAlertDescription::UnsupportedExtension, "A Certificate entry contains an unsolicited extension."_el};
        });

        try {
            // RFC 8446 section 4.4.2: each cert_data is one complete DER-encoded X.509 certificate.
            certificates.push_back(cryptology::X509Certificate::fromDerOrThrow(ByteBlock::fromSpan(certificateData)));
        } catch (const err::RuntimeError &error) {
            throw TlsProtocolError{TlsAlertDescription::BadCertificate, error.reason()};
        }
    }

    auto peerBundle =
        cryptology::X509CertificateBundle{util::List<cryptology::X509Certificate>{std::move(certificates)}};

    // RFC 8446 section 4.4.2.2: validate the chain, client purpose and time before
    // accepting CertificateVerify as authentication by the target key.
    _pendingCertificateValidation = TlsCertificateValidationRequest{
        *_selectedConfiguration.certificatePolicy, std::move(peerBundle), std::nullopt, time::Timestamp::now()};
    _transcript->update(message.span());
    _handshakeStep = HandshakeStep::ClientCertificateVerify;
    _checkpoint = TlsServerProtocolCheckpoint::CertificateValidation;
}

void TlsServerProtocol::completeCertificateValidation(cryptology::X509CertificateValidation validation) noexcept {
    if (_checkpoint != TlsServerProtocolCheckpoint::CertificateValidation || !_pendingCertificateValidation) {
        return;
    }
    _pendingCertificateValidation.reset();
    if (validation.isRejected()) {
        _certificateFailureCategory = validation.failure()->category();
        fail(
            TlsCertificateValidationRequest::alert(validation.failure()->category()),
            validation.failure()->diagnostic());
        return;
    }
    _validatedClientPath = validation.validatedPath();
    if (_validatedClientPath.isEmpty()) {
        fail(TlsAlertDescription::InternalError, "Certificate validation returned an empty accepted path."_el);
        return;
    }
    _checkpoint = TlsServerProtocolCheckpoint::None;
    processRetainedInputNoThrow();
}

void TlsServerProtocol::processClientCertificateVerify(const ByteBlock &message) {
    auto reader = TlsWireReader{message.span().subspan(4U)};
    const auto rawScheme = reader.readU16();
    const auto signature = reader.readVector16();
    reader.requireEnd();
    const auto scheme = cryptology::TlsSignatureScheme::fromRawValue(rawScheme);
    if (!scheme.has_value() || !scheme->isAllowedForCertificateVerify() || signature.empty() ||
        std::ranges::find(_clientSignatureSchemes, *scheme) == _clientSignatureSchemes.end()) {
        throw TlsProtocolError{
            TlsAlertDescription::IllegalParameter, "CertificateVerify selected an unsupported signature scheme."_el};
    }

    // RFC 8446 section 4.4.3: signed content is 64 spaces, the exact client context string, a zero separator, and the
    // transcript hash through Certificate. All of this verification input is public and needs no secure erasure.
    // anti-pattern: allow regular_string_literal -- Required ASCII data as bytes
    static constexpr auto cContext = std::string_view{"TLS 1.3, client CertificateVerify"};
    auto signedContent = ByteBlockEditor{unit::ByteCount{64U}, Byte{' '}};
    signedContent.append(toConstByteSpan(cContext));
    signedContent.append(Byte{0U});
    const auto transcriptHash = _transcript->hash();
    signedContent.append(transcriptHash);

    auto verified = false;
    try {
        verified = _validatedClientPath.first().publicKey().verifyTlsCertificateVerifySignature(
            *scheme, signedContent.span(), signature);
    } catch (const err::RuntimeError &) {
        verified = false;
    }
    if (!verified) {
        throw TlsProtocolError{TlsAlertDescription::DecryptError, "Client CertificateVerify is invalid."_el};
    }

    _transcript->update(message.span());
    _handshakeStep = HandshakeStep::ClientFinished;
}

}
