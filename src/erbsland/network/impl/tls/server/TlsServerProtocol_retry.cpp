#include "TlsServerProtocol.hpp"

#include "../TlsKeyShare.hpp"
#include "../TlsProtocolError.hpp"
#include "../TlsWireReader.hpp"
#include "../TlsWireWriter.hpp"

#include "../../../../cryptology/Hasher.hpp"
#include "../../../../text/Literals.hpp"

#include <map>

namespace erbsland::network::impl {

using namespace mem;
using namespace text::literals;

auto TlsServerProtocol::clientHelloInvariant(const ByteBlock &message) const -> ByteBlock {
    // RFC 8446 section 4.1.2: retain every fixed ClientHello field exactly, excluding the changing handshake length.
    auto reader = TlsWireReader{message.span().subspan(4U)};
    [[maybe_unused]] const auto versionAndRandom = reader.readBytes(34U);
    [[maybe_unused]] const auto sessionId = reader.readVector8();
    [[maybe_unused]] const auto suites = reader.readVector16();
    [[maybe_unused]] const auto compression = reader.readVector8();
    const auto fixedLength = message.span().size() - 4U - reader.remaining();
    auto hasher = cryptology::Hasher{cryptology::HashAlgorithm::Sha2_256};
    hasher.update(message.span().subspan(4U, fixedLength));
    const auto extensions = reader.readVector16();
    reader.requireEnd();
    auto retained = std::map<uint16_t, ByteBlock>{};
    forEachExtension(extensions, [&](const ExtensionView &extension) -> void {
        // This server never accepts PSK/early data or issues cookies. Only key_share and padding may change here.
        if (extension.type != 51U && extension.type != 21U) {
            retained.emplace(extension.type, ByteBlock::fromSpan(extension.data));
        }
    });
    for (const auto &[type, data] : retained) {
        auto writer = TlsWireWriter{};
        writer.writeU16(type);
        writer.writeVector16(data.span());
        const auto encoded = writer.finish();
        hasher.update(encoded.span());
    }
    return hasher.finalize();
}

void TlsServerProtocol::sendHelloRetryRequest(
    const ByteBlock &message,
    const cryptology::TlsCipherSuite suite,
    const cryptology::KeyAgreementAlgorithm group,
    const ConstByteSpan sessionId) {
    // RFC 8446 sections 4.1.4 and 4.2.8: request a compatible group only when no acceptable supplied share exists.
    if (_retryGroup.has_value() || !_handshakeStream.bufferedLength().isZero()) {
        throw TlsProtocolError{
            TlsAlertDescription::UnexpectedMessage, "Unexpected additional ClientHello before retry."_el};
    }
    auto version = TlsWireWriter{};
    version.writeU16(0x0304U);
    const auto versionBytes = version.finish();
    auto selectedGroup = TlsWireWriter{};
    selectedGroup.writeU16(tls_key_share::group(group));
    const auto groupBytes = selectedGroup.finish();
    auto extensions = TlsWireWriter{};
    extensions.writeU16(43U);
    extensions.writeVector16(versionBytes.span());
    extensions.writeU16(51U);
    extensions.writeVector16(groupBytes.span());
    const auto extensionBytes = extensions.finish();

    auto body = TlsWireWriter{};
    body.writeU16(0x0303U);
    body.writeBytes(tls_key_share::helloRetryRequestRandom().span());
    body.writeVector8(sessionId);
    body.writeU16(suite.toRawValue());
    body.writeU8(0U);
    body.writeVector16(extensionBytes.span());
    const auto bodyBytes = body.finish();
    auto handshake = TlsWireWriter{};
    handshake.writeU8(2U);
    handshake.writeU24(static_cast<uint32_t>(bodyBytes.length().toSizeT()));
    handshake.writeBytes(bodyBytes.span());
    const auto retry = handshake.finish();

    // RFC 8446 section 4.4.1: hash message_hash(ClientHello1) || HRR; no server private key exists yet.
    auto transcript = std::make_unique<cryptology::impl::Tls13Transcript>(suite.hashAlgorithm());
    transcript->update(message.span());
    transcript->retry(retry.span());
    _retryInvariant = clientHelloInvariant(message);
    _retryGroup = group;
    _cipherSuite = suite;
    _transcript = std::move(transcript);
    auto record = TlsWireWriter{};
    record.writeU8(22U);
    record.writeU16(0x0303U);
    record.writeVector16(retry.span());
    if (!queueTransport(record.finish())) {
        throw TlsProtocolError{
            TlsAlertDescription::InternalError, "The TLS output queue cannot hold HelloRetryRequest."_el};
    }
}

}
