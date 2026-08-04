// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsServerProtocol.hpp"

#include "TlsProtocolError.hpp"
#include "TlsWireWriter.hpp"

#include "../../cryptology/CryptologyError.hpp"
#include "../../cryptology/keys/KeyAgreementAlgorithm.hpp"
#include "../../cryptology/tls/TlsServerIdentity.hpp"
#include "../../cryptology/tls_record/TlsRecordContentType.hpp"
#include "../../err/ParameterError.hpp"
#include "../../mem/Byte.hpp"
#include "../../mem/ByteArray.hpp"
#include "../../mem/ByteBlockEditor.hpp"
#include "../../text/Literals.hpp"
#include "../../unit/ByteLength.hpp"

#include <algorithm>
#include <array>
#include <span>
#include <utility>

namespace erbsland::network::impl {

using namespace text::literals;

void TlsServerProtocol::buildServerFlight(mem::ByteBlock random, cryptology::KeyAgreementPrivateKey privateKey) {
    if (random.length() != unit::ByteLength{32U}) {
        throw err::ParameterError{"TLS ServerHello random must contain exactly 32 bytes."_el, "random"_el};
    }
    if (privateKey.isEmpty() || privateKey.algorithm() != cryptology::KeyAgreementAlgorithm::X25519) {
        throw err::ParameterError{"TLS server flight requires an X25519 private key."_el, "privateKey"_el};
    }
    auto flight = OutputFlight{};
    const auto serverPublicKey = privateKey.publicKey();

    // RFC 8446 Sections 4.1.3, 4.2.1, and 4.2.8: encode ServerHello in specification field order with only the
    // selected TLS 1.3 version and the server's fresh X25519 KeyShareEntry.
    auto serverHelloExtensions = TlsWireWriter{};
    serverHelloExtensions.writeU16(43U);
    serverHelloExtensions.writeVector16(mem::ByteArray<2U>{mem::Byte{0x03U}, mem::Byte{0x04U}}.span());
    auto serverKeyShare = TlsWireWriter{};
    serverKeyShare.writeU16(0x001dU);
    serverKeyShare.writeVector16(serverPublicKey.span());
    const auto serverKeyShareBytes = serverKeyShare.finish();
    serverHelloExtensions.writeU16(51U);
    serverHelloExtensions.writeVector16(serverKeyShareBytes.span());
    const auto serverHelloExtensionBytes = serverHelloExtensions.finish();

    auto serverHelloBody = TlsWireWriter{};
    serverHelloBody.writeU16(0x0303U);
    serverHelloBody.writeBytes(random.span());
    serverHelloBody.writeVector8(_legacySessionId.span());
    serverHelloBody.writeU16(_cipherSuite->toRawValue());
    serverHelloBody.writeU8(0U);
    serverHelloBody.writeVector16(serverHelloExtensionBytes.span());
    const auto serverHelloBodyBytes = serverHelloBody.finish();
    auto serverHelloWriter = TlsWireWriter{};
    serverHelloWriter.writeU8(2U);
    serverHelloWriter.writeU24(static_cast<uint32_t>(serverHelloBodyBytes.length().toSizeT()));
    serverHelloWriter.writeBytes(serverHelloBodyBytes.span());
    const auto serverHello = serverHelloWriter.finish();

    // RFC 8446 Section 5.1: ServerHello is the final plaintext handshake message and occupies one complete record.
    auto serverHelloRecord = TlsWireWriter{};
    serverHelloRecord.writeU8(22U);
    serverHelloRecord.writeU16(0x0303U);
    serverHelloRecord.writeVector16(serverHello.span());
    appendFlight(flight, serverHelloRecord.finish());

    // RFC 8446 Sections 4.4.1 and 7.1: transcript = ClientHello || ServerHello under the selected suite hash.
    _transcript = std::make_unique<cryptology::impl::Tls13Transcript>(_cipherSuite->hashAlgorithm());
    _transcript->update(_clientHello.span());
    _transcript->update(serverHello.span());

    // RFC 8446 Sections 4.2.8 and 7.1: transfer the ephemeral private key into visible protocol ownership, derive
    // (EC)DHE = X25519(server_private, client_public), erase the private key, then let the schedule consume and erase
    // the protected shared secret while deriving client/server handshake traffic secrets.
    _privateKey = std::move(privateKey);
    auto sharedSecret = cryptology::KeyAgreementSharedSecret{};
    try {
        sharedSecret = _privateKey.agree(_clientKey);
    } catch (const cryptology::CryptologyError &error) {
        // RFC 8446 Section 7.4.2 and RFC 7748 Section 6.1: an X25519 peer input that produces the all-zero output is
        // an illegal peer parameter. Translate the primitive's mandatory rejection into the protocol alert here.
        _privateKey.secureErase();
        throw TlsProtocolError{TlsAlertDescription::IllegalParameter, error.reason()};
    }
    _privateKey.secureErase();
    _keySchedule = std::make_unique<cryptology::impl::Tls13KeySchedule>(_cipherSuite->hashAlgorithm());
    const auto helloTranscriptHash = _transcript->hash();
    _keySchedule->initializeHandshake(std::move(sharedSecret), helloTranscriptHash.span());
    _encryptor = cryptology::TlsRecordEncryptor{*_cipherSuite, _keySchedule->handshakeTrafficSecret(false)};
    _decryptor = cryptology::TlsRecordDecryptor{*_cipherSuite, _keySchedule->handshakeTrafficSecret(true)};

    if (!_legacySessionId.isEmpty()) {
        // RFC 8446 Appendix D.4: a non-empty client compatibility session ID requires one dummy CCS immediately
        // after ServerHello. It is public compatibility framing and never enters the handshake transcript.
        auto ccs = TlsWireWriter{};
        ccs.writeU8(20U);
        ccs.writeU16(0x0303U);
        ccs.writeU16(1U);
        ccs.writeU8(1U);
        appendFlight(flight, ccs.finish());
    }

    // RFC 8446 Section 4.3.1, RFC 6066 Section 3, and RFC 7301 Section 3.2: acknowledge accepted SNI with an empty
    // response and serialize at most one selected ALPN protocol in EncryptedExtensions.
    auto encryptedExtensionValues = TlsWireWriter{};
    if (_serverName.has_value()) {
        encryptedExtensionValues.writeU16(0U);
        encryptedExtensionValues.writeVector16({});
    }
    if (!_negotiatedAlpn.isEmpty()) {
        auto selectedProtocols = TlsWireWriter{};
        selectedProtocols.writeVector8(_negotiatedAlpn.span());
        const auto selectedProtocolBytes = selectedProtocols.finish();
        auto alpnBody = TlsWireWriter{};
        alpnBody.writeVector16(selectedProtocolBytes.span());
        const auto alpnBodyBytes = alpnBody.finish();
        encryptedExtensionValues.writeU16(16U);
        encryptedExtensionValues.writeVector16(alpnBodyBytes.span());
    }
    const auto encryptedExtensionBytes = encryptedExtensionValues.finish();
    auto encryptedExtensionsBody = TlsWireWriter{};
    encryptedExtensionsBody.writeVector16(encryptedExtensionBytes.span());
    const auto encryptedExtensionsBodyBytes = encryptedExtensionsBody.finish();
    auto encryptedExtensionsWriter = TlsWireWriter{};
    encryptedExtensionsWriter.writeU8(8U);
    encryptedExtensionsWriter.writeU24(static_cast<uint32_t>(encryptedExtensionsBodyBytes.length().toSizeT()));
    encryptedExtensionsWriter.writeBytes(encryptedExtensionsBodyBytes.span());
    const auto encryptedExtensions = encryptedExtensionsWriter.finish();
    _transcript->update(encryptedExtensions.span());
    appendProtectedHandshake(flight, encryptedExtensions.span());

    // RFC 8446 Section 4.4.2: Certificate has an empty request context and the exact configured leaf-first X.509
    // chain. CertificateEntry extensions are empty because OCSP/SCT negotiation is outside this initial profile.
    auto certificateEntries = TlsWireWriter{};
    for (const auto &certificate : _selectedIdentity->certificateChain().certificates()) {
        const auto der = certificate.toDer();
        certificateEntries.writeVector24(der.span());
        certificateEntries.writeVector16({});
    }
    const auto certificateEntryBytes = certificateEntries.finish();
    auto certificateBody = TlsWireWriter{};
    certificateBody.writeVector8({});
    certificateBody.writeVector24(certificateEntryBytes.span());
    const auto certificateBodyBytes = certificateBody.finish();
    auto certificateWriter = TlsWireWriter{};
    certificateWriter.writeU8(11U);
    certificateWriter.writeU24(static_cast<uint32_t>(certificateBodyBytes.length().toSizeT()));
    certificateWriter.writeBytes(certificateBodyBytes.span());
    const auto certificate = certificateWriter.finish();
    _transcript->update(certificate.span());
    appendProtectedHandshake(flight, certificate.span());

    // RFC 8446 Section 4.4.3: signed_content = 64*0x20 || server_context || 0x00 ||
    // Transcript-Hash(ClientHello...Certificate). This input and the returned signature are public authenticators;
    // private signing material never leaves the protected immutable identity.
    // anti-pattern: allow regular_string_literal -- Required ASCII data as bytes
    static constexpr auto cContext = std::string_view{"TLS 1.3, server CertificateVerify"};
    auto signedContent = mem::ByteBlockEditor{unit::ByteLength{64U}, mem::Byte{' '}};
    signedContent.append(mem::toConstByteSpan(cContext));
    signedContent.append(mem::Byte{0U});
    const auto certificateTranscriptHash = _transcript->hash();
    signedContent.append(certificateTranscriptHash);
    const auto signature =
        _selectedIdentity->signingKey().signTlsCertificateVerify(*_signatureScheme, signedContent.span());

    auto certificateVerifyBody = TlsWireWriter{};
    certificateVerifyBody.writeU16(_signatureScheme->toRawValue());
    certificateVerifyBody.writeVector16(signature.span());
    const auto certificateVerifyBodyBytes = certificateVerifyBody.finish();
    auto certificateVerifyWriter = TlsWireWriter{};
    certificateVerifyWriter.writeU8(15U);
    certificateVerifyWriter.writeU24(static_cast<uint32_t>(certificateVerifyBodyBytes.length().toSizeT()));
    certificateVerifyWriter.writeBytes(certificateVerifyBodyBytes.span());
    const auto certificateVerify = certificateVerifyWriter.finish();
    _transcript->update(certificateVerify.span());
    appendProtectedHandshake(flight, certificateVerify.span());

    // RFC 8446 Section 4.4.4: server Finished verify_data =
    // HMAC(finished_key, Transcript-Hash(ClientHello...CertificateVerify)). Erase the sensitive temporary immediately
    // after copying it into the public authenticated handshake encoding.
    const auto beforeServerFinished = _transcript->hash();
    auto verifyData = _keySchedule->finishedVerifyData(false, beforeServerFinished.span());
    auto finishedWriter = TlsWireWriter{};
    finishedWriter.writeU8(20U);
    finishedWriter.writeU24(static_cast<uint32_t>(verifyData.length().toSizeT()));
    finishedWriter.writeBytes(verifyData.span());
    const auto finished = finishedWriter.finish();
    verifyData.secureErase();
    _transcript->update(finished.span());
    appendProtectedHandshake(flight, finished.span());

    // RFC 8446 Section 7.1: derive application traffic secrets from Transcript-Hash through server Finished. Build
    // the replacement server application write generation completely, then move-install it so the handshake write
    // generation is erased at this exact transition; client read keys remain at the handshake generation.
    const auto serverFinishedTranscriptHash = _transcript->hash();
    _keySchedule->initializeApplication(serverFinishedTranscriptHash.span());
    auto applicationEncryptor =
        cryptology::TlsRecordEncryptor{*_cipherSuite, _keySchedule->applicationTrafficSecret(false)};
    _encryptor = std::move(applicationEncryptor);

    // The full flight is already protected in bounded temporary storage. Commit all records together so local output
    // pressure cannot expose a syntactically valid but incomplete server flight.
    commitFlight(std::move(flight));
    _clientHello = {};
    _legacySessionId = {};
    _clientKey = {};
    _handshakeStep = HandshakeStep::ClientFinished;
}

void TlsServerProtocol::appendFlight(OutputFlight &flight, mem::ByteBlock record) const {
    const auto limit = _options.bufferLimits().send().toSizeT();
    const auto length = record.length().toSizeT();
    if (length > limit || _transportOutputBytes > limit - length ||
        flight.byteCount > limit - _transportOutputBytes - length) {
        throw TlsProtocolError{
            TlsAlertDescription::InternalError, "TLS output queue cannot hold the complete server flight."_el};
    }
    flight.byteCount += length;
    flight.records.push_back(std::move(record));
}

void TlsServerProtocol::appendProtectedHandshake(OutputFlight &flight, const mem::ConstByteSpan message) {
    // RFC 8446 Sections 4 and 5.1: preserve one complete transcript message while fragmenting only its record-layer
    // representation into authenticated fragments of at most 2^14 content bytes.
    auto offset = std::size_t{0U};
    do {
        const auto length = std::min<std::size_t>((std::size_t{1U} << 14U), message.size() - offset);
        auto record = _encryptor.protect(cryptology::TlsRecordContentType::Handshake, message.subspan(offset, length));
        appendFlight(flight, std::move(record));
        offset += length;
    } while (offset < message.size());
}

void TlsServerProtocol::commitFlight(OutputFlight &&flight) {
    if (!_transportOutput.empty() || _transportOutputBytes != 0U) {
        throw TlsProtocolError{
            TlsAlertDescription::InternalError, "TLS server flight cannot replace pending transport output."_el};
    }
    // The initial flight is the server's first output. A noexcept container swap transfers the already allocated
    // complete flight atomically, so an allocator failure cannot expose only a prefix of the local server flight.
    _transportOutput.swap(flight.records);
    _transportOutputBytes = flight.byteCount;
}

}
