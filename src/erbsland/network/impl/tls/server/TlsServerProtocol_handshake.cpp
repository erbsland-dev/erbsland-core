// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsServerProtocol.hpp"

#include "../TlsAlpnProtocol.hpp"
#include "../TlsProtocolError.hpp"
#include "../TlsWireReader.hpp"

#include "../../../../cryptology/keys/KeyAgreementAlgorithm.hpp"
#include "../../../../cryptology/tls/TlsServerIdentity.hpp"
#include "../../../../err/Exception.hpp"
#include "../../../../mem/Byte.hpp"
#include "../../../../text/Literals.hpp"

#include <set>
#include <string_view>
#include <utility>

namespace erbsland::network::impl {

using namespace text::literals;

void TlsServerProtocol::processHandshake(const mem::ByteBlock &message) {
    const auto type = message.getOrThrow(unit::ByteIndex::zero()).toUInt8();
    switch (_handshakeStep) {
    case HandshakeStep::ClientHello:
        if (type != 1U) {
            throw TlsProtocolError{TlsAlertDescription::UnexpectedMessage, "TLS server expected ClientHello."_el};
        }
        processClientHello(message);
        return;
    case HandshakeStep::ClientFinished:
        if (type != 20U) {
            throw TlsProtocolError{TlsAlertDescription::UnexpectedMessage, "TLS server expected client Finished."_el};
        }
        processClientFinished(message);
        return;
    case HandshakeStep::PostHandshake:
        processPostHandshake(message);
        return;
    }
}

void TlsServerProtocol::processClientHello(const mem::ByteBlock &message) {
    auto reader = TlsWireReader{message.span().subspan(4U)};

    // RFC 8446 Section 4.1.2: legacy_version is fixed to TLS 1.2 and random is exactly 32 public bytes.
    if (reader.readU16() != 0x0303U) {
        throw TlsProtocolError{TlsAlertDescription::ProtocolVersion, "ClientHello legacy_version is invalid."_el};
    }
    [[maybe_unused]] const auto clientRandom = reader.readBytes(32U);

    // RFC 8446 Section 4.1.2 and Appendix D.4: retain at most 32 compatibility session-ID bytes for the exact echo.
    const auto legacySessionId = reader.readVector8();
    if (legacySessionId.size() > 32U) {
        throw TlsProtocolError{TlsAlertDescription::IllegalParameter, "ClientHello session ID exceeds 32 bytes."_el};
    }

    // RFC 8446 Sections 4.1.1--4.1.2: parse the even non-empty offered suite vector under a fixed work bound, then
    // choose the first configured server-preference suite that the client offered.
    const auto cipherSuiteBytes = reader.readVector16();
    if (cipherSuiteBytes.empty() || (cipherSuiteBytes.size() % 2U) != 0U ||
        cipherSuiteBytes.size() / 2U > cMaximumListEntries) {
        throw TlsProtocolError{
            TlsAlertDescription::DecodeError, "ClientHello cipher suites are malformed or excessive."_el};
    }
    auto offeredSuites = std::set<uint16_t>{};
    auto suiteReader = TlsWireReader{cipherSuiteBytes};
    while (!suiteReader.isAtEnd()) {
        offeredSuites.insert(suiteReader.readU16());
    }
    auto selectedSuite = std::optional<cryptology::TlsCipherSuite>{};
    for (const auto suite : _options.cipherSuites()) {
        if (offeredSuites.contains(suite.toRawValue())) {
            selectedSuite = suite;
            break;
        }
    }
    if (!selectedSuite.has_value()) {
        throw TlsProtocolError{TlsAlertDescription::HandshakeFailure, "No configured TLS cipher suite was offered."_el};
    }

    // RFC 8446 Section 4.1.2: TLS 1.3 permits exactly the one-byte null legacy compression vector.
    const auto compressionMethods = reader.readVector8();
    if (compressionMethods.size() != 1U || compressionMethods[0] != mem::Byte{0U}) {
        throw TlsProtocolError{TlsAlertDescription::IllegalParameter, "ClientHello legacy compression is invalid."_el};
    }

    const auto extensionBytes = reader.readVector16();
    reader.requireEnd();
    auto hasTls13 = false;
    auto hasSignatureAlgorithms = false;
    auto hasSupportedGroups = false;
    auto hasX25519Group = false;
    auto hasKeyShare = false;
    auto offeredSchemes = std::vector<cryptology::TlsSignatureScheme>{};
    auto clientKey = cryptology::KeyAgreementPublicKey{};
    auto serverName = std::optional<HostName>{};
    auto offeredAlpn = std::vector<text::String>{};

    // RFC 8446 Section 4.2: parse every recognized extension exactly once; unknown extensions are ignored so future
    // public-input capabilities do not accidentally become required behavior.
    forEachExtension(extensionBytes, [&](const ExtensionView &extension) -> void {
        switch (extension.type) {
        case 0U: { // server_name
            // RFC 6066 Section 3: accept exactly one non-empty host_name and normalize its ASCII IDNA representation.
            auto extensionReader = TlsWireReader{extension.data};
            auto names = TlsWireReader{extensionReader.readVector16()};
            extensionReader.requireEnd();
            if (names.isAtEnd() || names.readU8() != 0U) {
                throw TlsProtocolError{TlsAlertDescription::DecodeError, "ClientHello SNI is malformed."_el};
            }
            const auto nameBytes = names.readVector16();
            if (nameBytes.empty() || !names.isAtEnd()) {
                throw TlsProtocolError{
                    TlsAlertDescription::IllegalParameter, "ClientHello must contain exactly one DNS SNI name."_el};
            }
            try {
                const auto view = std::string_view{reinterpret_cast<const char *>(nameBytes.data()), nameBytes.size()};
                serverName = HostName::fromStringOrThrow(text::String{view});
            } catch (const err::Exception &error) {
                throw TlsProtocolError{TlsAlertDescription::UnrecognizedName, error.reason()};
            }
            return;
        }
        case 10U: { // supported_groups
            // RFC 8446 Section 4.2.7: bound the even non-empty NamedGroup list and remember X25519 support.
            auto extensionReader = TlsWireReader{extension.data};
            auto groups = TlsWireReader{extensionReader.readVector16()};
            extensionReader.requireEnd();
            auto count = std::size_t{0U};
            while (!groups.isAtEnd()) {
                ++count;
                if (count > cMaximumListEntries) {
                    throw TlsProtocolError{
                        TlsAlertDescription::DecodeError, "ClientHello supported groups exceed the work bound."_el};
                }
                const auto group = groups.readU16();
                hasX25519Group = hasX25519Group || group == 0x001dU;
            }
            if (count == 0U) {
                throw TlsProtocolError{TlsAlertDescription::DecodeError, "ClientHello supported groups are empty."_el};
            }
            hasSupportedGroups = true;
            return;
        }
        case 13U: { // signature_algorithms
            // RFC 8446 Section 4.2.3: retain only known CertificateVerify schemes, preserving client preference.
            auto extensionReader = TlsWireReader{extension.data};
            auto schemes = TlsWireReader{extensionReader.readVector16()};
            extensionReader.requireEnd();
            auto count = std::size_t{0U};
            while (!schemes.isAtEnd()) {
                ++count;
                if (count > cMaximumListEntries) {
                    throw TlsProtocolError{
                        TlsAlertDescription::DecodeError, "ClientHello signature schemes exceed the work bound."_el};
                }
                const auto scheme = cryptology::TlsSignatureScheme::fromRawValue(schemes.readU16());
                if (scheme.has_value() && scheme->isAllowedForCertificateVerify()) {
                    offeredSchemes.push_back(*scheme);
                }
            }
            if (count == 0U) {
                throw TlsProtocolError{TlsAlertDescription::DecodeError, "ClientHello signature schemes are empty."_el};
            }
            hasSignatureAlgorithms = true;
            return;
        }
        case 16U: { // application_layer_protocol_negotiation
            // RFC 7301 Section 3.1: retain bounded, non-empty opaque protocol names in client order.
            auto extensionReader = TlsWireReader{extension.data};
            auto protocols = TlsWireReader{extensionReader.readVector16()};
            extensionReader.requireEnd();
            auto encodedLength = std::size_t{0U};
            while (!protocols.isAtEnd()) {
                if (offeredAlpn.size() >= cMaximumAlpnProtocols) {
                    throw TlsProtocolError{
                        TlsAlertDescription::DecodeError, "ClientHello ALPN exceeds 64 protocols."_el};
                }
                const auto protocol = protocols.readVector8();
                if (protocol.empty()) {
                    throw TlsProtocolError{
                        TlsAlertDescription::DecodeError, "ClientHello ALPN contains an empty protocol."_el};
                }
                encodedLength += 1U + protocol.size();
                if (encodedLength > cMaximumAlpnBytes) {
                    throw TlsProtocolError{TlsAlertDescription::DecodeError, "ClientHello ALPN exceeds 4096 bytes."_el};
                }
                offeredAlpn.push_back(TlsAlpnProtocol::fromBytes(protocol));
            }
            if (offeredAlpn.empty()) {
                throw TlsProtocolError{TlsAlertDescription::DecodeError, "ClientHello ALPN offer is empty."_el};
            }
            return;
        }
        case 41U: // pre_shared_key
            throw TlsProtocolError{
                TlsAlertDescription::UnsupportedExtension, "TLS pre-shared keys are not supported."_el};
        case 42U:   // early_data
            throw TlsProtocolError{TlsAlertDescription::UnsupportedExtension, "TLS early data is not supported."_el};
        case 43U: { // supported_versions
            // RFC 8446 Section 4.2.1: version negotiation uses the uint8 vector and must include TLS 1.3 (0x0304).
            auto extensionReader = TlsWireReader{extension.data};
            auto versions = TlsWireReader{extensionReader.readVector8()};
            extensionReader.requireEnd();
            auto count = std::size_t{0U};
            while (!versions.isAtEnd()) {
                ++count;
                if (count > cMaximumListEntries) {
                    throw TlsProtocolError{
                        TlsAlertDescription::DecodeError, "ClientHello versions exceed the work bound."_el};
                }
                const auto version = versions.readU16();
                hasTls13 = hasTls13 || version == 0x0304U;
            }
            if (count == 0U) {
                throw TlsProtocolError{
                    TlsAlertDescription::DecodeError, "ClientHello supported versions are empty."_el};
            }
            return;
        }
        case 44U: // cookie
            throw TlsProtocolError{
                TlsAlertDescription::UnsupportedExtension,
                "An unsolicited HelloRetryRequest cookie is not supported."_el};
        case 45U: { // psk_key_exchange_modes
            // RFC 8446 Section 4.2.9: validate the capability hint but ignore it because no actual PSK is accepted.
            auto extensionReader = TlsWireReader{extension.data};
            const auto modes = extensionReader.readVector8();
            extensionReader.requireEnd();
            if (modes.empty() || modes.size() > cMaximumListEntries) {
                throw TlsProtocolError{TlsAlertDescription::DecodeError, "ClientHello PSK modes are malformed."_el};
            }
            return;
        }
        case 50U: { // signature_algorithms_cert
            // RFC 8446 Section 4.2.3: structurally validate and bound certificate-signature preferences. This
            // single-identity increment has no alternate chain to select and sends its prevalidated configured chain.
            auto extensionReader = TlsWireReader{extension.data};
            auto schemes = TlsWireReader{extensionReader.readVector16()};
            extensionReader.requireEnd();
            auto count = std::size_t{0U};
            while (!schemes.isAtEnd()) {
                ++count;
                if (count > cMaximumListEntries) {
                    throw TlsProtocolError{
                        TlsAlertDescription::DecodeError, "Certificate signature schemes exceed the work bound."_el};
                }
                [[maybe_unused]] const auto rawScheme = schemes.readU16();
            }
            if (count == 0U) {
                throw TlsProtocolError{TlsAlertDescription::DecodeError, "Certificate signature schemes are empty."_el};
            }
            return;
        }
        case 51U: { // key_share
            // RFC 8446 Section 4.2.8: inspect at most 64 unique shares and retain the one exact 32-byte X25519 key.
            auto extensionReader = TlsWireReader{extension.data};
            auto shares = TlsWireReader{extensionReader.readVector16()};
            extensionReader.requireEnd();
            auto groups = std::set<uint16_t>{};
            auto count = std::size_t{0U};
            while (!shares.isAtEnd()) {
                ++count;
                if (count > cMaximumKeyShares) {
                    throw TlsProtocolError{
                        TlsAlertDescription::DecodeError, "ClientHello key shares exceed the work bound."_el};
                }
                const auto group = shares.readU16();
                const auto keyBytes = shares.readVector16();
                if (!groups.insert(group).second) {
                    throw TlsProtocolError{
                        TlsAlertDescription::IllegalParameter, "ClientHello repeats a key-share group."_el};
                }
                if (group == 0x001dU) {
                    if (keyBytes.size() != 32U) {
                        throw TlsProtocolError{
                            TlsAlertDescription::IllegalParameter, "ClientHello X25519 share has the wrong length."_el};
                    }
                    clientKey = cryptology::KeyAgreementPublicKey{cryptology::KeyAgreementAlgorithm::X25519, keyBytes};
                    hasKeyShare = true;
                }
            }
            if (count == 0U) {
                throw TlsProtocolError{TlsAlertDescription::DecodeError, "ClientHello key shares are empty."_el};
            }
            return;
        }
        default:
            return;
        }
    });

    if (!hasTls13) {
        throw TlsProtocolError{TlsAlertDescription::ProtocolVersion, "ClientHello does not offer TLS 1.3."_el};
    }
    if (!hasSignatureAlgorithms || !hasSupportedGroups) {
        throw TlsProtocolError{
            TlsAlertDescription::MissingExtension,
            "ClientHello is missing signature_algorithms or supported_groups."_el};
    }
    if (!hasX25519Group || !hasKeyShare) {
        // RFC 8446 Sections 4.1.4 and 4.2.8: another compatible share would require HelloRetryRequest, deliberately
        // absent from the initial profile, so fail before any expensive key agreement.
        throw TlsProtocolError{
            TlsAlertDescription::HandshakeFailure, "ClientHello would require HelloRetryRequest."_el};
    }

    auto selectedIdentity = _options.defaultIdentity();
    auto selectedIdentityIndex = std::size_t{};
    if (serverName.has_value()) {
        const auto &namedIdentities = _options.namedIdentities();
        for (auto index = std::size_t{}; index < namedIdentities.size(); ++index) {
            if (namedIdentities[index].serverName == *serverName) {
                selectedIdentity = namedIdentities[index].identity;
                selectedIdentityIndex = index + 1U;
                break;
            }
        }
    }
    const auto selectedSignature =
        selectedIdentity->selectSignatureScheme(util::List<cryptology::TlsSignatureScheme>{std::move(offeredSchemes)});
    if (!selectedSignature.has_value()) {
        throw TlsProtocolError{
            TlsAlertDescription::HandshakeFailure, "No compatible CertificateVerify scheme was offered."_el};
    }

    auto negotiatedAlpn = text::String{};
    if (!offeredAlpn.empty()) {
        for (const auto &supported : _options.alpnProtocols()) {
            for (const auto &offered : offeredAlpn) {
                if (TlsAlpnProtocol::equal(supported, offered)) {
                    negotiatedAlpn = supported;
                    break;
                }
            }
            if (!negotiatedAlpn.isEmpty()) {
                break;
            }
        }
        if (negotiatedAlpn.isEmpty()) {
            throw TlsProtocolError{
                TlsAlertDescription::NoApplicationProtocol, "No configured ALPN protocol was offered."_el};
        }
    }

    // RFC 8446 Sections 5.1 and 5.2: ClientHello is the last plaintext handshake message before the key boundary.
    if (!_handshakeStream.bufferedLength().isZero()) {
        throw TlsProtocolError{
            TlsAlertDescription::UnexpectedMessage, "Handshake bytes cross the ClientHello key boundary."_el};
    }

    _clientHello = message;
    _legacySessionId = mem::ByteBlock::fromSpan(legacySessionId);
    _clientKey = std::move(clientKey);
    _cipherSuite = *selectedSuite;
    _signatureScheme = *selectedSignature;
    _selectedIdentity = std::move(selectedIdentity);
    _selectedIdentityIndex = selectedIdentityIndex;
    _serverName = std::move(serverName);
    _offeredAlpn = std::move(offeredAlpn);
    _negotiatedAlpn = std::move(negotiatedAlpn);
    _checkpoint = TlsServerProtocolCheckpoint::ClientHello;
}

void TlsServerProtocol::processClientFinished(const mem::ByteBlock &message) {
    const auto verifyData = message.span().subspan(4U);
    const auto beforeClientFinished = _transcript->hash();

    // RFC 8446 Sections 5.1 and 5.2: Finished is the last message protected by client handshake traffic keys. No
    // following post-handshake message may be coalesced across this record-protection boundary.
    if (!_handshakeStream.bufferedLength().isZero()) {
        throw TlsProtocolError{
            TlsAlertDescription::UnexpectedMessage, "Handshake bytes cross the client Finished key boundary."_el};
    }

    // RFC 8446 Section 4.4.4: expected_verify_data =
    // HMAC(client_finished_key, Transcript-Hash(ClientHello...server Finished)); compare the complete authenticator in
    // constant time before adding client Finished to the authenticated transcript.
    if (verifyData.size() != _cipherSuite->hashAlgorithm().digestSize().toSizeT() ||
        !_keySchedule->verifyFinished(true, beforeClientFinished.span(), verifyData)) {
        throw TlsProtocolError{TlsAlertDescription::DecryptError, "Client Finished verify_data is invalid."_el};
    }
    _transcript->update(message.span());

    // RFC 8446 Section 7.1: build client application read state from client_application_traffic_secret_0, install it
    // over the consumed client handshake read generation, then derive and immediately discard the unused resumption
    // master secret from Transcript-Hash through client Finished.
    auto applicationDecryptor =
        cryptology::TlsRecordDecryptor{*_cipherSuite, _keySchedule->applicationTrafficSecret(true)};
    _decryptor = std::move(applicationDecryptor);
    const auto clientFinishedTranscriptHash = _transcript->hash();
    _keySchedule->discardResumptionMaster(clientFinishedTranscriptHash.span());

    _handshakeStep = HandshakeStep::PostHandshake;
    _state = TlsServerProtocolState::Established;
    _checkpoint = TlsServerProtocolCheckpoint::HandshakeCompleted;
}

void TlsServerProtocol::forEachExtension(const mem::ConstByteSpan extensions, const ExtensionFn &function) const {
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

}
