// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsClientProtocol.hpp"

#include "../TlsAlpnProtocol.hpp"
#include "../TlsExtensionRules.hpp"
#include "../TlsKeyShare.hpp"
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
#include "../../../../text/impl/UnsafeU8StringBuffer.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../unit/ByteCount.hpp"
#include "../../../../unit/ByteIndex.hpp"

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

void TlsClientProtocol::processHandshake(const ByteBlock &message) {
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

void TlsClientProtocol::processServerHello(const ByteBlock &message) {
    auto reader = TlsWireReader{message.span().subspan(4U)};

    // RFC 8446 section 4.1.3: legacy_version remains 0x0303 and random distinguishes ServerHello from HRR.
    if (reader.readU16() != 0x0303U) {
        throw TlsProtocolError{TlsAlertDescription::ProtocolVersion, "ServerHello legacy_version is invalid."_el};
    }
    const auto serverRandom = reader.readBytes(32U);
    const auto isRetry = ByteBlock::fromSpan(serverRandom) == tls_key_share::helloRetryRequestRandom();

    // RFC 8446 appendix D.4: compatibility mode requires the exact legacy session ID echo.
    if (ByteBlock::fromSpan(reader.readVector8()) != _legacySessionId) {
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
    if (_retryReceived && selectedSuite != _cipherSuite) {
        throw TlsProtocolError{TlsAlertDescription::IllegalParameter, "ServerHello changed the retry cipher suite."_el};
    }
    if (isRetry) {
        processHelloRetryRequest(message, *selectedSuite, extensionBytes);
        return;
    }
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
            const auto algorithm = tls_key_share::algorithm(extensionReader.readU16());
            if (!algorithm.has_value() || *algorithm != _privateKey.algorithm()) {
                throw TlsProtocolError{
                    TlsAlertDescription::IllegalParameter, "ServerHello selected an unoffered key share."_el};
            }
            const auto keyBytes = extensionReader.readVector16();
            extensionReader.requireEnd();
            try {
                serverKey = cryptology::KeyAgreementPublicKey{*algorithm, keyBytes};
            } catch (const err::Exception &) {
                throw TlsProtocolError{
                    TlsAlertDescription::IllegalParameter, "ServerHello contains an invalid key share."_el};
            }
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
    if (!_retryReceived) {
        _transcript = std::make_unique<cryptology::impl::Tls13Transcript>(_cipherSuite->hashAlgorithm());
        _transcript->update(_clientHello.span());
    }
    _transcript->update(message.span());
    _clientHello = {};
    _legacySessionId = {};

    // RFC 8446 section 7.1: consume ECDHE and install both handshake traffic directions. Erase the ephemeral private
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

void TlsClientProtocol::processEncryptedExtensions(const ByteBlock &message) {
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
            if (_options.alpnProtocols().isEmpty()) {
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
            const auto selectedProtocol = text::impl::UnsafeU8StringBuffer{selected}.takeString();
            auto wasOffered = false;
            for (const auto &offered : _options.alpnProtocols()) {
                if (tls_alpn_protocol::equal(offered, selectedProtocol)) {
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

void TlsClientProtocol::processCertificateRequest(const ByteBlock &message) {
    auto reader = TlsWireReader{message.span().subspan(4U)};

    // RFC 8446 section 4.4.2.1: a main-handshake CertificateRequest has an empty certificate_request_context.
    if (!reader.readVector8().empty()) {
        throw TlsProtocolError{
            TlsAlertDescription::IllegalParameter, "Main-handshake CertificateRequest context must be empty."_el};
    }
    const auto extensionBytes = reader.readVector16();
    reader.requireEnd();
    auto hasSignatureAlgorithms = false;
    auto hasCertificateSignatureAlgorithms = false;
    forEachExtension(extensionBytes, [&](const ExtensionView &extension) -> void {
        if (extension.type == 13U || extension.type == 50U) {
            auto extensionReader = TlsWireReader{extension.data};
            const auto schemes = extensionReader.readVector16();
            extensionReader.requireEnd();
            if (schemes.empty() || (schemes.size() % 2U) != 0U) {
                throw TlsProtocolError{
                    TlsAlertDescription::DecodeError, "CertificateRequest signature schemes are malformed."_el};
            }
            auto selected = std::vector<cryptology::TlsSignatureScheme>{};
            auto schemesReader = TlsWireReader{schemes};
            while (!schemesReader.isAtEnd()) {
                const auto scheme = cryptology::TlsSignatureScheme::fromRawValue(schemesReader.readU16());
                if (scheme.has_value()) {
                    selected.push_back(*scheme);
                }
            }
            if (extension.type == 13U) {
                _requestedSignatureSchemes = util::List<cryptology::TlsSignatureScheme>{std::move(selected)};
            } else {
                hasCertificateSignatureAlgorithms = true;
                _requestedCertificateSchemes = util::List<cryptology::TlsSignatureScheme>{std::move(selected)};
            }
            hasSignatureAlgorithms = hasSignatureAlgorithms || extension.type == 13U;
            return;
        }
        if (extension.type == 47U) {
            // RFC 8446 section 4.2.4: retain the complete non-empty DistinguishedName vector for exact issuer
            // constraints before any configured client identity can be disclosed.
            auto extensionReader = TlsWireReader{extension.data};
            const auto authorities = extensionReader.readVector16();
            extensionReader.requireEnd();
            if (authorities.size() < 3U) {
                throw TlsProtocolError{
                    TlsAlertDescription::DecodeError, "CertificateRequest authorities are malformed."_el};
            }
            auto authoritiesReader = TlsWireReader{authorities};
            while (!authoritiesReader.isAtEnd()) {
                const auto authority = authoritiesReader.readVector16();
                _requestedAuthorities.push_back(ByteBlock::fromSpan(authority));
                if (authority.empty()) {
                    throw TlsProtocolError{
                        TlsAlertDescription::DecodeError, "CertificateRequest contains an empty authority name."_el};
                }
            }
            return;
        }
        if (extension.type == 48U) {
            // RFC 8446 section 4.2.5: parse each OIDFilter exactly; unsupported nonempty constraints suppress
            // client identity disclosure rather than silently broadening the server's accepted identity set.
            auto extensionReader = TlsWireReader{extension.data};
            auto filtersReader = TlsWireReader{extensionReader.readVector16()};
            extensionReader.requireEnd();
            _hasUnsupportedOidFilters = !filtersReader.isAtEnd();
            while (!filtersReader.isAtEnd()) {
                if (filtersReader.readVector8().empty()) {
                    throw TlsProtocolError{
                        TlsAlertDescription::DecodeError, "CertificateRequest contains an empty OID filter."_el};
                }
                [[maybe_unused]] const auto values = filtersReader.readVector16();
            }
            return;
        }
        tls_extension_rules::requireIgnorableCertificateRequest(extension.type);
    });
    if (!hasSignatureAlgorithms) {
        throw TlsProtocolError{
            TlsAlertDescription::MissingExtension, "CertificateRequest is missing signature_algorithms."_el};
    }

    if (!hasCertificateSignatureAlgorithms) {
        _requestedCertificateSchemes = _requestedSignatureSchemes;
    }
    _transcript->update(message.span());
    _certificateRequested = true;
    _handshakeStep = HandshakeStep::Certificate;
}

}
