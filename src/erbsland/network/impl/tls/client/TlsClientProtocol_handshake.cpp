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
#include "../../../../mem/Byte.hpp"
#include "../../../../mem/ByteArray.hpp"
#include "../../../../mem/ByteBlockEditor.hpp"
#include "../../../../mem/ByteSpan.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../unit/ByteIndex.hpp"
#include "../../../../unit/ByteLength.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <set>
#include <span>
#include <utility>
#include <vector>

namespace erbsland::network::impl {

using namespace text::literals;

void TlsClientProtocol::processHandshake(const mem::ByteBlock &message) {
    const auto type = message.getOrThrow(unit::ByteIndex::zero()).toUInt8();
    switch (_handshakeStep) {
    case HandshakeStep::ServerHello:
        if (type != 2U) {
            throw TlsProtocolError{TlsAlertDescription::UnexpectedMessage, "Expected TLS ServerHello."_el};
        }
        processServerHello(message);
        break;
    case HandshakeStep::EncryptedExtensions:
        if (type != 8U) {
            throw TlsProtocolError{TlsAlertDescription::UnexpectedMessage, "Expected TLS EncryptedExtensions."_el};
        }
        processEncryptedExtensions(message);
        break;
    case HandshakeStep::CertificateOrRequest:
        if (type == 13U) {
            processCertificateRequest(message);
        } else if (type == 11U) {
            processCertificate(message);
        } else {
            throw TlsProtocolError{
                TlsAlertDescription::UnexpectedMessage, "Expected TLS CertificateRequest or Certificate."_el};
        }
        break;
    case HandshakeStep::Certificate:
        if (type != 11U) {
            throw TlsProtocolError{TlsAlertDescription::UnexpectedMessage, "Expected TLS Certificate."_el};
        }
        processCertificate(message);
        break;
    case HandshakeStep::CertificateVerify:
        if (type != 15U) {
            throw TlsProtocolError{TlsAlertDescription::UnexpectedMessage, "Expected TLS CertificateVerify."_el};
        }
        processCertificateVerify(message);
        break;
    case HandshakeStep::ServerFinished:
        if (type != 20U) {
            throw TlsProtocolError{TlsAlertDescription::UnexpectedMessage, "Expected TLS Finished."_el};
        }
        processServerFinished(message);
        break;
    case HandshakeStep::PostHandshake:
        processPostHandshake(message);
        break;
    }
}

void TlsClientProtocol::processServerHello(const mem::ByteBlock &message) {
    static constexpr auto cHelloRetryRequestRandom = mem::ByteArray<32U>{
        mem::Byte{0xcfU},
        mem::Byte{0x21U},
        mem::Byte{0xadU},
        mem::Byte{0x74U},
        mem::Byte{0xe5U},
        mem::Byte{0x9aU},
        mem::Byte{0x61U},
        mem::Byte{0x11U},
        mem::Byte{0xbeU},
        mem::Byte{0x1dU},
        mem::Byte{0x8cU},
        mem::Byte{0x02U},
        mem::Byte{0x1eU},
        mem::Byte{0x65U},
        mem::Byte{0xb8U},
        mem::Byte{0x91U},
        mem::Byte{0xc2U},
        mem::Byte{0xa2U},
        mem::Byte{0x11U},
        mem::Byte{0x16U},
        mem::Byte{0x7aU},
        mem::Byte{0xbbU},
        mem::Byte{0x8cU},
        mem::Byte{0x5eU},
        mem::Byte{0x07U},
        mem::Byte{0x9eU},
        mem::Byte{0x09U},
        mem::Byte{0xe2U},
        mem::Byte{0xc8U},
        mem::Byte{0xa8U},
        mem::Byte{0x33U},
        mem::Byte{0x9cU}};

    auto reader = TlsWireReader{message.span().subspan(4U)};

    // RFC 8446 section 4.1.3: legacy_version remains 0x0303 and random distinguishes ServerHello from HRR.
    if (reader.readU16() != 0x0303U) {
        throw TlsProtocolError{TlsAlertDescription::ProtocolVersion, "ServerHello legacy_version is invalid."_el};
    }
    const auto serverRandom = reader.readBytes(32U);
    if (mem::ByteBlock::fromSpan(serverRandom) == mem::ByteBlock{cHelloRetryRequestRandom}) {
        // The client already offered its only supported group and initial share; accepting HRR would add no compatible
        // transition and is deliberately deferred with multi-group support.
        throw TlsProtocolError{TlsAlertDescription::UnexpectedMessage, "HelloRetryRequest is not supported."_el};
    }

    // RFC 8446 appendix D.4: compatibility mode requires the exact legacy session ID echo.
    if (mem::ByteBlock::fromSpan(reader.readVector8()) != _legacySessionId) {
        throw TlsProtocolError{TlsAlertDescription::IllegalParameter, "ServerHello did not echo the session ID."_el};
    }

    // RFC 8446 section 4.1.3: the server selects exactly one offered TLS 1.3 suite and null compression.
    const auto selectedSuite = cryptology::TlsCipherSuite::fromRawValue(reader.readU16());
    if (!selectedSuite.has_value()) {
        throw TlsProtocolError{TlsAlertDescription::IllegalParameter, "ServerHello selected an unsupported suite."_el};
    }
    if (reader.readU8() != 0U) {
        throw TlsProtocolError{TlsAlertDescription::IllegalParameter, "ServerHello selected legacy compression."_el};
    }

    const auto extensionBytes = reader.readVector16();
    reader.requireEnd();
    auto hasSupportedVersion = false;
    auto hasKeyShare = false;
    auto serverKey = cryptology::KeyAgreementPublicKey{};
    forEachExtension(extensionBytes, [&](const ExtensionView &extension) -> void {
        if (extension.type == 43U) {
            auto extensionReader = TlsWireReader{extension.data};
            if (extensionReader.readU16() != 0x0304U) {
                throw TlsProtocolError{TlsAlertDescription::ProtocolVersion, "ServerHello did not select TLS 1.3."_el};
            }
            extensionReader.requireEnd();
            hasSupportedVersion = true;
            return;
        }
        if (extension.type == 51U) {
            auto extensionReader = TlsWireReader{extension.data};
            if (extensionReader.readU16() != 0x001dU) {
                throw TlsProtocolError{TlsAlertDescription::IllegalParameter, "ServerHello did not select X25519."_el};
            }
            const auto keyBytes = extensionReader.readVector16();
            extensionReader.requireEnd();
            if (keyBytes.size() != 32U) {
                throw TlsProtocolError{
                    TlsAlertDescription::IllegalParameter, "ServerHello X25519 share has the wrong length."_el};
            }
            serverKey = cryptology::KeyAgreementPublicKey{cryptology::KeyAgreementAlgorithm::X25519, keyBytes};
            hasKeyShare = true;
            return;
        }
        throw TlsProtocolError{
            TlsAlertDescription::UnsupportedExtension, "ServerHello contains an unsolicited extension."_el};
    });
    if (!hasSupportedVersion || !hasKeyShare) {
        throw TlsProtocolError{
            TlsAlertDescription::MissingExtension, "ServerHello is missing supported_versions or key_share."_el};
    }

    // RFC 8446 sections 5.1 and 5.2: the record-protection epoch changes immediately after ServerHello. Bytes for a
    // later handshake message therefore cannot be coalesced behind ServerHello in the same plaintext record.
    if (!_handshakeStream.bufferedLength().isZero()) {
        throw TlsProtocolError{
            TlsAlertDescription::UnexpectedMessage, "Handshake bytes cross the ServerHello key boundary."_el};
    }

    // RFC 8446 sections 4.4.1 and 7.1: select the suite hash, then hash the exact ClientHello and ServerHello.
    _cipherSuite = *selectedSuite;
    _transcript = std::make_unique<cryptology::impl::Tls13Transcript>(_cipherSuite->hashAlgorithm());
    _transcript->update(_clientHello.span());
    _transcript->update(message.span());
    _clientHello = {};
    _legacySessionId = {};

    // RFC 8446 section 7.1: consume X25519 and install both handshake traffic directions. Erase the ephemeral private
    // key immediately after agreement; the schedule consumes and erases the protected shared secret.
    auto sharedSecret = _privateKey.agree(serverKey);
    _privateKey.secureErase();
    _keySchedule = std::make_unique<cryptology::impl::Tls13KeySchedule>(_cipherSuite->hashAlgorithm());
    const auto transcriptHash = _transcript->hash();
    _keySchedule->initializeHandshake(std::move(sharedSecret), transcriptHash.span());
    _encryptor = cryptology::TlsRecordEncryptor{*_cipherSuite, _keySchedule->handshakeTrafficSecret(true)};
    _decryptor = cryptology::TlsRecordDecryptor{*_cipherSuite, _keySchedule->handshakeTrafficSecret(false)};
    _handshakeStep = HandshakeStep::EncryptedExtensions;
}

void TlsClientProtocol::processEncryptedExtensions(const mem::ByteBlock &message) {
    auto reader = TlsWireReader{message.span().subspan(4U)};
    const auto extensionBytes = reader.readVector16();
    reader.requireEnd();

    forEachExtension(extensionBytes, [&](const ExtensionView &extension) -> void {
        if (extension.type == 0U) {
            if (!extension.data.empty() || !_options.host().isName()) {
                throw TlsProtocolError{
                    TlsAlertDescription::UnsupportedExtension, "The encrypted server_name response is invalid."_el};
            }
            return;
        }
        if (extension.type == 10U) {
            // RFC 8446 section 4.2.7 permits the server to advertise its supported groups in EncryptedExtensions.
            auto extensionReader = TlsWireReader{extension.data};
            const auto groups = extensionReader.readVector16();
            extensionReader.requireEnd();
            if (groups.empty() || (groups.size() % 2U) != 0U) {
                throw TlsProtocolError{
                    TlsAlertDescription::DecodeError, "The encrypted supported_groups value is malformed."_el};
            }
            return;
        }
        if (extension.type == 16U) {
            if (_options.alpnProtocols().empty()) {
                throw TlsProtocolError{
                    TlsAlertDescription::UnsupportedExtension, "The server selected unoffered ALPN."_el};
            }
            auto extensionReader = TlsWireReader{extension.data};
            auto protocols = TlsWireReader{extensionReader.readVector16()};
            extensionReader.requireEnd();
            const auto selected = protocols.readVector8();
            if (selected.empty() || !protocols.isAtEnd()) {
                throw TlsProtocolError{
                    TlsAlertDescription::DecodeError, "The ALPN response must select exactly one protocol."_el};
            }
            const auto selectedProtocol = TlsAlpnProtocol::fromBytes(selected);
            auto wasOffered = false;
            for (const auto &offered : _options.alpnProtocols()) {
                if (TlsAlpnProtocol::equal(offered, selectedProtocol)) {
                    wasOffered = true;
                    break;
                }
            }
            if (!wasOffered) {
                throw TlsProtocolError{
                    TlsAlertDescription::IllegalParameter, "The server selected an ALPN protocol not offered."_el};
            }
            _negotiatedAlpn = selectedProtocol;
            return;
        }
        throw TlsProtocolError{
            TlsAlertDescription::UnsupportedExtension, "EncryptedExtensions contains an unsolicited extension."_el};
    });

    // RFC 8446 section 4.4.1: authenticate the exact EncryptedExtensions encoding in the transcript after parsing.
    _transcript->update(message.span());
    _handshakeStep = HandshakeStep::CertificateOrRequest;
    _checkpoint = TlsClientProtocolCheckpoint::PeerHello;
}

void TlsClientProtocol::processCertificateRequest(const mem::ByteBlock &message) {
    auto reader = TlsWireReader{message.span().subspan(4U)};

    // RFC 8446 section 4.4.2.1: a main-handshake CertificateRequest has an empty certificate_request_context.
    if (!reader.readVector8().empty()) {
        throw TlsProtocolError{
            TlsAlertDescription::IllegalParameter, "Main-handshake CertificateRequest context must be empty."_el};
    }
    const auto extensionBytes = reader.readVector16();
    reader.requireEnd();
    auto hasSignatureAlgorithms = false;
    forEachExtension(extensionBytes, [&](const ExtensionView &extension) -> void {
        if (extension.type == 13U || extension.type == 50U) {
            auto extensionReader = TlsWireReader{extension.data};
            const auto schemes = extensionReader.readVector16();
            extensionReader.requireEnd();
            if (schemes.empty() || (schemes.size() % 2U) != 0U) {
                throw TlsProtocolError{
                    TlsAlertDescription::DecodeError, "CertificateRequest signature schemes are malformed."_el};
            }
            hasSignatureAlgorithms = hasSignatureAlgorithms || extension.type == 13U;
            return;
        }
        if (extension.type == 47U) {
            // RFC 8446 section 4.2.4: parse the complete non-empty DistinguishedName vector even though this client
            // has no configured identity and therefore does not use the authorities for certificate selection.
            auto extensionReader = TlsWireReader{extension.data};
            const auto authorities = extensionReader.readVector16();
            extensionReader.requireEnd();
            if (authorities.size() < 3U) {
                throw TlsProtocolError{
                    TlsAlertDescription::DecodeError, "CertificateRequest authorities are malformed."_el};
            }
            auto authoritiesReader = TlsWireReader{authorities};
            while (!authoritiesReader.isAtEnd()) {
                if (authoritiesReader.readVector16().empty()) {
                    throw TlsProtocolError{
                        TlsAlertDescription::DecodeError, "CertificateRequest contains an empty authority name."_el};
                }
            }
            return;
        }
        if (extension.type == 48U) {
            // RFC 8446 section 4.2.5: parse each OIDFilter representation exactly. Its selection semantics are
            // intentionally unused because the initial client always returns an empty certificate list.
            auto extensionReader = TlsWireReader{extension.data};
            auto filtersReader = TlsWireReader{extensionReader.readVector16()};
            extensionReader.requireEnd();
            while (!filtersReader.isAtEnd()) {
                if (filtersReader.readVector8().empty()) {
                    throw TlsProtocolError{
                        TlsAlertDescription::DecodeError, "CertificateRequest contains an empty OID filter."_el};
                }
                [[maybe_unused]] const auto values = filtersReader.readVector16();
            }
            return;
        }
        throw TlsProtocolError{
            TlsAlertDescription::UnsupportedExtension, "CertificateRequest contains an unsupported extension."_el};
    });
    if (!hasSignatureAlgorithms) {
        throw TlsProtocolError{
            TlsAlertDescription::MissingExtension, "CertificateRequest is missing signature_algorithms."_el};
    }

    _transcript->update(message.span());
    _certificateRequested = true;
    _handshakeStep = HandshakeStep::Certificate;
}

}
