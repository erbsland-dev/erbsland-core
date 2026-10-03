// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsClientHelloBuilder.hpp"

#include "../TlsKeyShare.hpp"

#include "../../../../cryptology/keys/KeyAgreementAlgorithm.hpp"
#include "../../../../cryptology/tls/TlsCipherSuite.hpp"
#include "../../../../cryptology/tls/TlsSignatureScheme.hpp"
#include "../../../../err/ParameterFailure.hpp"
#include "../../../../mem/Byte.hpp"
#include "../../../../mem/ByteArray.hpp"
#include "../../../../mem/ByteSpan.hpp"
#include "../../../../text/impl/UnsafeU8StringAccess.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../text/StringList.hpp"
#include "../../../../unit/ItemCount.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <utility>

namespace erbsland::network::impl {

using namespace err;
using namespace mem;
using namespace text::literals;

TlsClientHelloBuilder::TlsClientHelloBuilder(
    Host host, text::StringList alpnProtocols, std::vector<cryptology::KeyAgreementAlgorithm> groups) :
    _groups{std::move(groups)}, _host{std::move(host)}, _alpnProtocols{std::move(alpnProtocols)} {
    tls_key_share::validate(_groups);
    // RFC 7301 section 3.1 uses one-byte ProtocolName lengths. The additional count and aggregate bounds keep local
    // configuration from creating an unexpectedly large ClientHello and match the protocol parser's peer bounds.
    if (_alpnProtocols.count() > unit::ItemCount{32U}) {
        throw ParameterFailure{"At most 32 ALPN protocols can be offered."_el, "alpnProtocols"_el};
    }
    auto encodedLength = std::size_t{0U};
    for (const auto &protocol : _alpnProtocols) {
        const auto bytes = tls_alpn_protocol::bytes(protocol);
        if (bytes.empty() || bytes.size() > std::numeric_limits<uint8_t>::max()) {
            throw ParameterFailure{"Each ALPN protocol must contain between 1 and 255 bytes."_el, "alpnProtocols"_el};
        }
        encodedLength += 1U + bytes.size();
    }
    if (encodedLength > 4096U) {
        throw ParameterFailure{"The encoded ALPN offer exceeds 4096 bytes."_el, "alpnProtocols"_el};
    }
}

auto TlsClientHelloBuilder::buildHandshake(
    const ConstByteSpan random,
    const ConstByteSpan legacySessionId,
    const cryptology::KeyAgreementPublicKey &publicKey,
    const ConstByteSpan cookie) const -> ByteBlock {
    if (random.size() != 32U || legacySessionId.size() != 32U) {
        throw ParameterFailure{
            "TLS middlebox-compatible ClientHello random and session ID must each contain 32 bytes."_el, "random"_el};
    }
    if (publicKey.isEmpty() || std::find(_groups.begin(), _groups.end(), publicKey.algorithm()) == _groups.end()) {
        throw ParameterFailure{"TLS ClientHello requires a public key for an offered group."_el, "publicKey"_el};
    }

    auto body = TlsWireWriter{};

    // RFC 8446 section 4.1.2: legacy_version is 0x0303 and random contains exactly 32 fresh bytes.
    body.writeU16(0x0303U);
    body.writeBytes(random);

    // RFC 8446 appendix D.4: a non-empty unpredictable legacy_session_id enables middlebox compatibility mode.
    body.writeVector8(legacySessionId);

    // RFC 8446 sections 4.1.2 and 9.1: offer only the three supported TLS 1.3 suites in library preference order.
    auto cipherSuites = TlsWireWriter{};
    for (const auto suite : cryptology::TlsCipherSuite::all()) {
        cipherSuites.writeU16(suite.toRawValue());
    }
    const auto cipherSuiteBytes = cipherSuites.finish();
    body.writeVector16(cipherSuiteBytes.span());

    // RFC 8446 section 4.1.2: legacy_compression_methods is exactly one null compression method.
    static constexpr auto cNullCompression = ByteArray<1U>{Byte{0U}};
    body.writeVector8(cNullCompression.span());

    // RFC 8446 section 4.1.2: append the complete extension vector after every fixed ClientHello field.
    const auto extensions = buildExtensions(publicKey, cookie);
    body.writeVector16(extensions.span());
    const auto bodyBytes = body.finish();

    // RFC 8446 section 4: Handshake = msg_type client_hello || uint24 body length || ClientHello.
    auto handshake = TlsWireWriter{};
    handshake.writeU8(1U);
    handshake.writeU24(static_cast<uint32_t>(bodyBytes.length().toSizeT()));
    handshake.writeBytes(bodyBytes.span());
    return handshake.finish();
}

auto TlsClientHelloBuilder::buildInitialRecord(const ConstByteSpan handshake) const -> ByteBlock {
    if (handshake.empty() || handshake.size() > (std::size_t{1U} << 14U)) {
        throw ParameterFailure{
            "The initial ClientHello must fit in one non-empty TLSPlaintext fragment."_el, "handshake"_el};
    }

    auto record = TlsWireWriter{};
    // RFC 8446 appendix D.4: the first ClientHello record uses handshake type and legacy_record_version 0x0301.
    record.writeU8(22U);
    record.writeU16(0x0301U);
    record.writeU16(static_cast<uint16_t>(handshake.size()));
    record.writeBytes(handshake);
    return record.finish();
}

auto TlsClientHelloBuilder::buildExtensions(
    const cryptology::KeyAgreementPublicKey &publicKey, const ConstByteSpan cookie) const -> ByteBlock {
    auto extensions = TlsWireWriter{};

    if (const auto name = _host.name(); name.has_value()) {
        // RFC 6066 section 3: server_name is ServerNameList<1..2^16-1> containing one host_name with uint16 bytes.
        const auto asciiName = name->toString(HostNameFormat::IdnaAscii);
        const auto nameData = text::impl::UnsafeU8StringAccess{asciiName}.dataSpan();
        const auto nameBytes = toConstByteSpan(nameData);
        auto serverName = TlsWireWriter{};
        serverName.writeU8(0U);
        serverName.writeVector16(nameBytes);
        const auto serverNameBytes = serverName.finish();
        auto serverNameList = TlsWireWriter{};
        serverNameList.writeVector16(serverNameBytes.span());
        const auto body = serverNameList.finish();
        appendExtension(extensions, 0U, body.span());
    }

    // RFC 8446 section 4.2.7: supported_groups retains the configured preference order across retries.
    auto groups = TlsWireWriter{};
    for (const auto algorithm : _groups) {
        groups.writeU16(tls_key_share::group(algorithm));
    }
    const auto groupValues = groups.finish();
    auto supportedGroups = TlsWireWriter{};
    supportedGroups.writeVector16(groupValues.span());
    const auto supportedGroupsBody = supportedGroups.finish();
    appendExtension(extensions, 10U, supportedGroupsBody.span());

    // RFC 8446 section 4.2.3: signature_algorithms contains only schemes permitted for CertificateVerify.
    auto verifySchemes = TlsWireWriter{};
    const auto allSchemes = {
        cryptology::TlsSignatureScheme{cryptology::TlsSignatureScheme::RsaPssRsaeSha256},
        cryptology::TlsSignatureScheme{cryptology::TlsSignatureScheme::RsaPssRsaeSha512},
        cryptology::TlsSignatureScheme{cryptology::TlsSignatureScheme::RsaPssRsaeSha384},
        cryptology::TlsSignatureScheme{cryptology::TlsSignatureScheme::EcdsaSecp256r1Sha256},
        cryptology::TlsSignatureScheme{cryptology::TlsSignatureScheme::EcdsaSecp384r1Sha384},
        cryptology::TlsSignatureScheme{cryptology::TlsSignatureScheme::Ed25519},
        cryptology::TlsSignatureScheme{cryptology::TlsSignatureScheme::RsaPssPssSha256},
        cryptology::TlsSignatureScheme{cryptology::TlsSignatureScheme::RsaPssPssSha512},
        cryptology::TlsSignatureScheme{cryptology::TlsSignatureScheme::RsaPssPssSha384},
    };
    for (const auto scheme : allSchemes) {
        verifySchemes.writeU16(scheme.toRawValue());
    }
    const auto verifySchemeValues = verifySchemes.finish();
    auto signatureAlgorithms = TlsWireWriter{};
    signatureAlgorithms.writeVector16(verifySchemeValues.span());
    const auto signatureAlgorithmsBody = signatureAlgorithms.finish();
    appendExtension(extensions, 13U, signatureAlgorithmsBody.span());

    // RFC 8446 section 4.2.3: signature_algorithms_cert additionally permits PKCS#1 certificate signatures.
    auto certificateSchemes = TlsWireWriter{};
    for (const auto scheme : allSchemes) {
        certificateSchemes.writeU16(scheme.toRawValue());
    }
    certificateSchemes.writeU16(cryptology::TlsSignatureScheme::RsaPkcs1Sha256);
    certificateSchemes.writeU16(cryptology::TlsSignatureScheme::RsaPkcs1Sha384);
    certificateSchemes.writeU16(cryptology::TlsSignatureScheme::RsaPkcs1Sha512);
    const auto certificateSchemeValues = certificateSchemes.finish();
    auto signatureAlgorithmsCert = TlsWireWriter{};
    signatureAlgorithmsCert.writeVector16(certificateSchemeValues.span());
    const auto signatureAlgorithmsCertBody = signatureAlgorithmsCert.finish();
    appendExtension(extensions, 50U, signatureAlgorithmsCertBody.span());

    if (!_alpnProtocols.isEmpty()) {
        // RFC 7301 section 3.1: ProtocolNameList is a non-empty uint16 vector of opaque non-empty uint8 names.
        auto protocolNames = TlsWireWriter{};
        for (const auto &protocol : _alpnProtocols) {
            protocolNames.writeVector8(tls_alpn_protocol::bytes(protocol));
        }
        const auto protocolNameValues = protocolNames.finish();
        auto alpn = TlsWireWriter{};
        alpn.writeVector16(protocolNameValues.span());
        const auto alpnBody = alpn.finish();
        appendExtension(extensions, 16U, alpnBody.span());
    }

    // RFC 8446 section 4.2.1: supported_versions is a uint8 vector containing only TLS 1.3 (0x0304).
    static constexpr auto cTls13Version = ByteArray<2U>{Byte{0x03U}, Byte{0x04U}};
    auto versions = TlsWireWriter{};
    versions.writeVector8(cTls13Version.span());
    const auto versionsBody = versions.finish();
    appendExtension(extensions, 43U, versionsBody.span());

    // RFC 8446 section 4.2.8: KeyShareClientHello contains one X25519 KeyShareEntry.
    auto keyShareEntry = TlsWireWriter{};
    keyShareEntry.writeU16(tls_key_share::group(publicKey.algorithm()));
    keyShareEntry.writeVector16(publicKey.span());
    const auto keyShareEntryBytes = keyShareEntry.finish();
    auto keyShare = TlsWireWriter{};
    keyShare.writeVector16(keyShareEntryBytes.span());
    const auto keyShareBody = keyShare.finish();
    appendExtension(extensions, 51U, keyShareBody.span());

    // RFC 8446 section 4.2.2: echo the opaque retry cookie only in ClientHello2.
    if (!cookie.empty()) {
        auto cookieBody = TlsWireWriter{};
        cookieBody.writeVector16(cookie);
        const auto encodedCookie = cookieBody.finish();
        appendExtension(extensions, 44U, encodedCookie.span());
    }
    return extensions.finish();
}

void TlsClientHelloBuilder::appendExtension(TlsWireWriter &extensions, const uint16_t type, const ConstByteSpan body) {
    // RFC 8446 section 4.2: Extension is uint16 type followed by opaque extension_data<0..2^16-1>.
    extensions.writeU16(type);
    extensions.writeVector16(body);
}

}
