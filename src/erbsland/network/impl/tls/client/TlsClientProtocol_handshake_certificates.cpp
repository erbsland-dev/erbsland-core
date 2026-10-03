// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsClientProtocol.hpp"

#include "../TlsAlpnProtocol.hpp"
#include "../TlsProtocolError.hpp"
#include "../TlsWireReader.hpp"
#include "../TlsWireWriter.hpp"

#include "../../../../cryptology/keys/KeyAgreementAlgorithm.hpp"
#include "../../../../cryptology/keys/KeyAgreementPublicKey.hpp"
#include "../../../../cryptology/tls/TlsSignatureScheme.hpp"
#include "../../../../cryptology/tls_record/TlsRecordContentType.hpp"
#include "../../../../cryptology/x509/X509CertificateBundle.hpp"
#include "../../../../cryptology/x509/X509CertificateValidationFailureCategory.hpp"
#include "../../../../err/Exception.hpp"
#include "../../../../err/RuntimeError.hpp"
#include "../../../../mem/Byte.hpp"
#include "../../../../mem/ByteArray.hpp"
#include "../../../../mem/ByteBlockEditor.hpp"
#include "../../../../mem/ByteSpan.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../unit/ByteCount.hpp"
#include "../../../../unit/ByteIndex.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <set>
#include <span>
#include <utility>
#include <vector>

namespace erbsland::network::impl {

using namespace mem;
using namespace text::literals;

void TlsClientProtocol::processCertificate(const ByteBlock &message) {
    auto reader = TlsWireReader{message.span().subspan(4U)};

    // RFC 8446 section 4.4.2: the server Certificate in the main handshake uses the empty request context.
    if (!reader.readVector8().empty()) {
        throw TlsProtocolError{
            TlsAlertDescription::IllegalParameter, "The server Certificate request context must be empty."_el};
    }
    const auto certificateList = reader.readVector24();
    reader.requireEnd();
    if (certificateList.empty() || certificateList.size() > 1024U * 1024U) {
        throw TlsProtocolError{
            TlsAlertDescription::BadCertificate, "The server Certificate list is empty or exceeds one MiB."_el};
    }

    auto listReader = TlsWireReader{certificateList};
    auto certificates = std::vector<cryptology::X509Certificate>{};
    while (!listReader.isAtEnd()) {
        if (certificates.size() >= cMaximumCertificates) {
            throw TlsProtocolError{
                TlsAlertDescription::BadCertificate, "The server Certificate list exceeds 16 entries."_el};
        }
        const auto certificateData = listReader.readVector24();
        if (certificateData.empty()) {
            throw TlsProtocolError{TlsAlertDescription::BadCertificate, "A TLS Certificate entry is empty."_el};
        }
        const auto certificateExtensions = listReader.readVector16();
        forEachExtension(certificateExtensions, [&](const ExtensionView &) -> void {
            // No certificate-entry extension was offered by this initial client, so any response is unsolicited.
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

    // RFC 8446 section 4.4.2.2: validate the chain, server purpose, time, and original DNS/IP identity before
    // accepting CertificateVerify as authentication by the target key.
    _pendingCertificateValidation = TlsCertificateValidationRequest{
        _options.certificatePolicy(), std::move(peerBundle), _options.host(), _options.validationTime()};
    _transcript->update(message.span());
    _handshakeStep = HandshakeStep::CertificateVerify;
    _checkpoint = TlsClientProtocolCheckpoint::CertificateValidation;
}

void TlsClientProtocol::completeCertificateValidation(cryptology::X509CertificateValidation validation) noexcept {
    if (_checkpoint != TlsClientProtocolCheckpoint::CertificateValidation || !_pendingCertificateValidation) {
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
    _validatedPath = validation.validatedPath();
    if (_validatedPath.isEmpty()) {
        fail(TlsAlertDescription::InternalError, "Certificate validation returned an empty accepted path."_el);
        return;
    }
    _serverPublicKey = _validatedPath.toRawValue().front().publicKey();
    _checkpoint = TlsClientProtocolCheckpoint::None;
    processRetainedInputNoThrow();
}

void TlsClientProtocol::processCertificateVerify(const ByteBlock &message) {
    auto reader = TlsWireReader{message.span().subspan(4U)};
    const auto rawScheme = reader.readU16();
    const auto signature = reader.readVector16();
    reader.requireEnd();
    const auto scheme = cryptology::TlsSignatureScheme::fromRawValue(rawScheme);
    if (!scheme.has_value() || !scheme->isAllowedForCertificateVerify() || signature.empty()) {
        throw TlsProtocolError{
            TlsAlertDescription::IllegalParameter, "CertificateVerify selected an unsupported signature scheme."_el};
    }

    // RFC 8446 section 4.4.3: signed content is 64 spaces, the exact server context string, a zero separator, and the
    // transcript hash through Certificate. All of this verification input is public and needs no secure erasure.
    // anti-pattern: allow regular_string_literal -- Required ASCII data as bytes
    static constexpr auto cContext = std::string_view{"TLS 1.3, server CertificateVerify"};
    auto signedContent = ByteBlockEditor{unit::ByteCount{64U}, Byte{' '}};
    signedContent.append(toConstByteSpan(cContext));
    signedContent.append(Byte{0U});
    const auto transcriptHash = _transcript->hash();
    signedContent.append(transcriptHash);

    auto verified = false;
    try {
        verified = _serverPublicKey.verifyTlsCertificateVerifySignature(*scheme, signedContent.span(), signature);
    } catch (const err::RuntimeError &) {
        verified = false;
    }
    if (!verified) {
        throw TlsProtocolError{TlsAlertDescription::DecryptError, "Server CertificateVerify is invalid."_el};
    }

    _transcript->update(message.span());
    _handshakeStep = HandshakeStep::ServerFinished;
    _checkpoint = TlsClientProtocolCheckpoint::PeerAuthenticated;
}

void TlsClientProtocol::processServerFinished(const ByteBlock &message) {
    const auto verifyData = message.span().subspan(4U);
    const auto beforeServerFinished = _transcript->hash();

    // RFC 8446 sections 5.1 and 5.2: Finished is the last message protected by server handshake traffic keys. Reject
    // coalesced bytes so no post-handshake message can be authenticated under the preceding protection epoch.
    if (!_handshakeStream.bufferedLength().isZero()) {
        throw TlsProtocolError{
            TlsAlertDescription::UnexpectedMessage, "Handshake bytes cross the server Finished key boundary."_el};
    }

    // RFC 8446 section 4.4.4: verify server Finished in constant time before adding it to the authenticated transcript.
    if (verifyData.size() != _cipherSuite->hashAlgorithm().digestSize().toSizeT() ||
        !_keySchedule->verifyFinished(false, beforeServerFinished.span(), verifyData)) {
        throw TlsProtocolError{TlsAlertDescription::DecryptError, "Server Finished verify_data is invalid."_el};
    }
    _transcript->update(message.span());

    // RFC 8446 section 7.1: application traffic and exporter secrets bind the transcript through server Finished.
    const auto serverFinishedTranscriptHash = _transcript->hash();
    _keySchedule->initializeApplication(serverFinishedTranscriptHash.span());

    // RFC 8446 appendix D.4: send one plaintext dummy CCS immediately before the encrypted client second flight.
    queueCompatibilityCcs();

    if (_certificateRequested) {
        // RFC 8446 section 4.4.2: emit the suitable configured identity, or an empty list when constraints fail.
        auto certificateBody = TlsWireWriter{};
        certificateBody.writeVector8(ConstByteSpan{});
        _clientSignatureScheme = selectClientIdentity();
        auto entries = TlsWireWriter{};
        if (_clientSignatureScheme.has_value()) {
            for (const auto &certificate : _options.clientIdentity()->certificateChain().certificates()) {
                const auto der = certificate.toDer();
                entries.writeVector24(der.span());
                entries.writeVector16({});
            }
        }
        const auto encodedEntries = entries.finish();
        certificateBody.writeVector24(encodedEntries.span());
        const auto certificateBodyBytes = certificateBody.finish();
        auto certificate = TlsWireWriter{};
        certificate.writeU8(11U);
        certificate.writeU24(static_cast<uint32_t>(certificateBodyBytes.length().toSizeT()));
        certificate.writeBytes(certificateBodyBytes.span());
        const auto certificateMessage = certificate.finish();
        _transcript->update(certificateMessage.span());
        queueProtectedHandshake(certificateMessage.span());
    }

    if (_clientSignatureScheme.has_value()) {
        // RFC 8446 section 4.4.3: disclose only the explicitly configured suitable identity after authenticating
        // server CertificateVerify and Finished. The request retains only public content and provider ownership.
        // anti-pattern: allow regular_string_literal -- Required ASCII data as bytes
        static constexpr auto cContext = std::string_view{"TLS 1.3, client CertificateVerify"};
        auto content = ByteBlockEditor{unit::ByteCount{64U}, Byte{' '}};
        content.append(toConstByteSpan(cContext));
        content.append(Byte{0U});
        content.append(_transcript->hash());
        _pendingSignature = TlsSignatureRequest{_options.clientIdentity(), *_clientSignatureScheme, ByteBlock{content}};
        _checkpoint = TlsClientProtocolCheckpoint::Signature;
        return;
    }
    finishClientFlight();
}

void TlsClientProtocol::finishClientFlight() {
    auto applicationDecryptor =
        cryptology::TlsRecordDecryptor{*_cipherSuite, _keySchedule->applicationTrafficSecret(false)};
    auto applicationEncryptor =
        cryptology::TlsRecordEncryptor{*_cipherSuite, _keySchedule->applicationTrafficSecret(true)};
    // RFC 8446 section 4.4.4: client Finished covers the transcript after any client authentication messages.
    const auto beforeClientFinished = _transcript->hash();
    auto clientVerifyData = _keySchedule->finishedVerifyData(true, beforeClientFinished.span());
    auto finished = TlsWireWriter{};
    finished.writeU8(20U);
    finished.writeU24(static_cast<uint32_t>(clientVerifyData.length().toSizeT()));
    finished.writeBytes(clientVerifyData.span());
    clientVerifyData.secureErase();
    const auto finishedMessage = finished.finish();
    queueProtectedHandshake(finishedMessage.span());
    _transcript->update(finishedMessage.span());

    // RFC 8446 sections 4.4.4 and 7.1: client write keys switch only after its Finished record is constructed. The
    // old handshake encryptor and server decryptor erase their generations when replaced by the application objects.
    _encryptor = std::move(applicationEncryptor);
    _decryptor = std::move(applicationDecryptor);
    const auto clientFinishedTranscriptHash = _transcript->hash();
    _keySchedule->discardResumptionMaster(clientFinishedTranscriptHash.span());
    _handshakeStep = HandshakeStep::PostHandshake;
    _state = TlsClientProtocolState::Established;
    _checkpoint = TlsClientProtocolCheckpoint::HandshakeCompleted;
}

void TlsClientProtocol::forEachExtension(const ConstByteSpan extensions, const ExtensionFn &function) const {
    auto reader = TlsWireReader{extensions};
    auto seen = std::set<uint16_t>{};
    auto count = std::size_t{0U};
    while (!reader.isAtEnd()) {
        ++count;
        if (count > cMaximumExtensions) {
            throw TlsProtocolError{
                TlsAlertDescription::DecodeError, "A TLS message exceeds the 64-extension bound."_el};
        }
        const auto type = reader.readU16();
        const auto data = reader.readVector16();
        if (!seen.insert(type).second) {
            throw TlsProtocolError{
                TlsAlertDescription::IllegalParameter, "A TLS message contains a duplicate extension."_el};
        }
        function(ExtensionView{type, data});
    }
}

void TlsClientProtocol::queueProtectedHandshake(const ConstByteSpan message) {
    // RFC 8446 sections 4 and 5.2: fragment certificate chains at 2^14 bytes, reserving the complete flight's
    // encoded space before consuming sequence numbers. Identity data and signatures are public.
    const auto count = (message.size() + 16383U) / 16384U;
    const auto recordLength = message.size() + count * 22U;
    const auto limit = _options.bufferLimits().send().toSizeT();
    if (recordLength > limit || _transportOutputBytes > limit - recordLength) {
        throw TlsProtocolError{
            TlsAlertDescription::InternalError, "The TLS output queue cannot hold the handshake flight."_el};
    }
    auto offset = std::size_t{};
    while (offset < message.size()) {
        const auto length = std::min<std::size_t>(16384U, message.size() - offset);
        auto record = _encryptor.protect(cryptology::TlsRecordContentType::Handshake, message.subspan(offset, length));
        if (!queueTransport(std::move(record))) {
            throw TlsProtocolError{
                TlsAlertDescription::InternalError, "The preflighted TLS handshake record was not queued."_el};
        }
        offset += length;
    }
}

}
