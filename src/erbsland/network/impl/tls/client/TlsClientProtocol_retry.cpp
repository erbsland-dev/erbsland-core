#include "TlsClientProtocol.hpp"

#include "../TlsKeyShare.hpp"
#include "../TlsProtocolError.hpp"
#include "../TlsWireReader.hpp"
#include "../TlsWireWriter.hpp"

#include "../../../../text/Literals.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::network::impl {

using namespace mem;
using namespace text::literals;

void TlsClientProtocol::processHelloRetryRequest(
    const ByteBlock &message, const cryptology::TlsCipherSuite suite, const ConstByteSpan extensions) {
    // RFC 8446 section 4.1.4: a second HRR, or one coalesced with another handshake transition, is forbidden.
    if (_retryReceived || !_handshakeStream.bufferedLength().isZero()) {
        throw TlsProtocolError{TlsAlertDescription::UnexpectedMessage, "Unexpected additional HelloRetryRequest."_el};
    }
    auto hasVersion = false;
    auto group = std::optional<cryptology::KeyAgreementAlgorithm>{};
    auto cookie = ByteBlock{};
    forEachExtension(extensions, [&](const ExtensionView &extension) -> void {
        auto reader = TlsWireReader{extension.data};
        if (extension.type == 43U) {
            if (reader.readU16() != 0x0304U) {
                throw TlsProtocolError{
                    TlsAlertDescription::ProtocolVersion, "HelloRetryRequest did not select TLS 1.3."_el};
            }
            reader.requireEnd();
            hasVersion = true;
        } else if (extension.type == 51U) {
            group = tls_key_share::algorithm(reader.readU16());
            reader.requireEnd();
            const auto &offered = _options.supportedGroups();
            if (!group.has_value() || *group == _privateKey.algorithm() ||
                std::find(offered.begin(), offered.end(), *group) == offered.end()) {
                throw TlsProtocolError{
                    TlsAlertDescription::IllegalParameter, "HelloRetryRequest selected an invalid group."_el};
            }
        } else if (extension.type == 44U) {
            const auto value = reader.readVector16();
            reader.requireEnd();
            // RFC 8446 section 4.2.2: the cookie is opaque and nonempty. A 60 KiB local bound leaves room for
            // the immutable ClientHello extensions within the uint16 extension vector and bounds echo work.
            if (value.empty() || value.size() > 60U * 1024U) {
                throw TlsProtocolError{
                    TlsAlertDescription::IllegalParameter, "HelloRetryRequest cookie is empty or excessive."_el};
            }
            cookie = ByteBlock::fromSpan(value);
        } else {
            throw TlsProtocolError{
                TlsAlertDescription::UnsupportedExtension, "HelloRetryRequest contains an unsolicited extension."_el};
        }
    });
    if (!hasVersion) {
        throw TlsProtocolError{
            TlsAlertDescription::MissingExtension, "HelloRetryRequest is missing supported_versions."_el};
    }
    if (!group.has_value() && cookie.isEmpty()) {
        throw TlsProtocolError{
            TlsAlertDescription::IllegalParameter, "HelloRetryRequest would not change ClientHello."_el};
    }

    // RFC 8446 sections 4.1.2 and 4.2.8: only a requested group change replaces the ephemeral private key.
    // Fully construct the replacement and ClientHello2 first; cookie-only retries retain the original private key.
    auto replacement = cryptology::KeyAgreementPrivateKey{};
    auto publicKey = _privateKey.publicKey();
    if (group.has_value()) {
        replacement = cryptology::KeyAgreementPrivateKey::generate(*group);
        publicKey = replacement.publicKey();
    }
    const auto random = _clientHello.span().subspan(6U, 32U);
    const auto secondHello =
        _clientHelloBuilder.buildHandshake(random, _legacySessionId.span(), publicKey, cookie.span());
    auto transcript = std::make_unique<cryptology::impl::Tls13Transcript>(suite.hashAlgorithm());
    transcript->update(_clientHello.span());
    transcript->retry(message.span());
    transcript->update(secondHello.span());
    if (group.has_value()) {
        auto previous = std::move(_privateKey);
        _privateKey = std::move(replacement);
        previous.secureErase();
    }
    _transcript = std::move(transcript);
    _cipherSuite = suite;
    _retryReceived = true;
    _clientHello = {};

    // RFC 8446 appendix D.4 and section 5.1: emit compatibility CCS, then fragment ClientHello2 into TLSPlaintext.
    queueCompatibilityCcs();
    auto remaining = secondHello.span();
    while (!remaining.empty()) {
        const auto length = std::min(remaining.size(), std::size_t{1U << 14U});
        auto record = TlsWireWriter{};
        record.writeU8(22U);
        record.writeU16(0x0303U);
        record.writeVector16(remaining.first(length));
        if (!queueTransport(record.finish())) {
            throw TlsProtocolError{
                TlsAlertDescription::InternalError, "The TLS output queue cannot hold ClientHello2."_el};
        }
        remaining = remaining.subspan(length);
    }
}

void TlsClientProtocol::queueCompatibilityCcs() {
    if (_compatibilityCcsSent) {
        return;
    }
    auto ccs = TlsWireWriter{};
    ccs.writeU8(20U);
    ccs.writeU16(0x0303U);
    ccs.writeU16(1U);
    ccs.writeU8(1U);
    if (!queueTransport(ccs.finish())) {
        throw TlsProtocolError{TlsAlertDescription::InternalError, "The TLS output queue cannot hold client CCS."_el};
    }
    _compatibilityCcsSent = true;
}

}
