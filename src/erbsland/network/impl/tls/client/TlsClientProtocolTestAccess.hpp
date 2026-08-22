// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsClientProtocol.hpp"

#include "../TlsProtocolError.hpp"
#include "../TlsWireWriter.hpp"

#include "../../../../cryptology/tls_record/TlsRecordDecryptor.hpp"
#include "../../../../cryptology/tls_record/TlsRecordEncryptor.hpp"
#include "../../../../text/Literals.hpp"

#include <utility>

namespace erbsland::network::impl {

/// Deterministic test-only access to TLS protocol inputs and independently protected peer directions.
/// @tested{TlsClientProtocolTest}
class TlsClientProtocolTestAccess final {
public:
    /// Bind one protocol core.
    explicit TlsClientProtocolTestAccess(TlsClientProtocol &protocol) noexcept : _protocol{protocol} {}

public:
    /// Start with exact deterministic random, session ID, and X25519 private key.
    void start(mem::ByteBlock random, mem::ByteBlock legacySessionId, cryptology::KeyAgreementPrivateKey privateKey) {
        _protocol.startWithInputs(std::move(random), std::move(legacySessionId), std::move(privateKey));
    }
    /// Create an independent server handshake sender from the negotiated schedule.
    [[nodiscard]] auto serverHandshakeEncryptor() const -> cryptology::TlsRecordEncryptor {
        return cryptology::TlsRecordEncryptor{
            *_protocol._cipherSuite, _protocol._keySchedule->handshakeTrafficSecret(false)};
    }
    /// Create an independent client handshake receiver from the negotiated schedule.
    [[nodiscard]] auto clientHandshakeDecryptor() const -> cryptology::TlsRecordDecryptor {
        return cryptology::TlsRecordDecryptor{
            *_protocol._cipherSuite, _protocol._keySchedule->handshakeTrafficSecret(true)};
    }
    /// Advance across certificate validation and CertificateVerify after independently testing those cryptographic
    /// components. The exact supplied complete messages remain part of the transcript.
    void acceptServerAuthentication(mem::ConstByteSpan certificate, mem::ConstByteSpan certificateVerify) {
        using namespace text::literals;
        if (_protocol._handshakeStep != TlsClientProtocol::HandshakeStep::CertificateOrRequest) {
            throw TlsProtocolError{
                TlsAlertDescription::InternalError, "Test authentication bypass used at the wrong handshake step."_el};
        }
        _protocol._transcript->update(certificate);
        _protocol._transcript->update(certificateVerify);
        _protocol._handshakeStep = TlsClientProtocol::HandshakeStep::ServerFinished;
        _protocol._checkpoint = TlsClientProtocolCheckpoint::PeerAuthenticated;
    }
    /// Advance across CertificateVerify after the protocol parsed and validated a real Certificate message.
    /// The exact supplied complete CertificateVerify message remains part of the transcript.
    void acceptCertificateVerify(mem::ConstByteSpan certificateVerify) {
        using namespace text::literals;
        if (_protocol._handshakeStep != TlsClientProtocol::HandshakeStep::CertificateVerify) {
            throw TlsProtocolError{
                TlsAlertDescription::InternalError,
                "Test CertificateVerify bypass used at the wrong handshake step."_el};
        }
        _protocol._transcript->update(certificateVerify);
        _protocol._handshakeStep = TlsClientProtocol::HandshakeStep::ServerFinished;
        _protocol._checkpoint = TlsClientProtocolCheckpoint::PeerAuthenticated;
    }
    /// Construct server Finished for the current test transcript without advancing it.
    [[nodiscard]] auto serverFinishedMessage() const -> mem::ByteBlock {
        const auto transcriptHash = _protocol._transcript->hash();
        auto verifyData = _protocol._keySchedule->finishedVerifyData(false, transcriptHash.span());
        auto writer = TlsWireWriter{};
        writer.writeU8(20U);
        writer.writeU24(static_cast<uint32_t>(verifyData.length().toSizeT()));
        writer.writeBytes(verifyData.span());
        verifyData.secureErase();
        return writer.finish();
    }
    /// Create an independent server application sender from generation zero.
    [[nodiscard]] auto serverApplicationEncryptor() const -> cryptology::TlsRecordEncryptor {
        return cryptology::TlsRecordEncryptor{
            *_protocol._cipherSuite, _protocol._keySchedule->applicationTrafficSecret(false)};
    }
    /// Create an independent client application receiver from generation zero.
    [[nodiscard]] auto clientApplicationDecryptor() const -> cryptology::TlsRecordDecryptor {
        return cryptology::TlsRecordDecryptor{
            *_protocol._cipherSuite, _protocol._keySchedule->applicationTrafficSecret(true)};
    }

private:
    TlsClientProtocol &_protocol; ///< Protocol under test.
};

}
