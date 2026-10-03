// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Tls13Transcript.hpp"

#include "../../../err/ParameterFailure.hpp"
#include "../../../mem/ByteArray.hpp"
#include "../../../text/Literals.hpp"

namespace erbsland::cryptology::impl {

using namespace mem;
using namespace text::literals;

Tls13Transcript::Tls13Transcript(const HashAlgorithm algorithm) : _hasher{algorithm} {
}

Tls13Transcript::~Tls13Transcript() {
    secureErase();
}

void Tls13Transcript::update(const ConstByteSpan message) {
    // RFC 8446 section 4: every Handshake value has msg_type || uint24 length || body. Validate this framing before
    // adding bytes so a truncated or coalesced caller value cannot silently create a different transcript.
    if (message.size() < 4U) {
        throw err::ParameterFailure{"A complete TLS handshake header is required."_el, "message"_el};
    }
    const auto bodyLength = (static_cast<std::size_t>(message[1].toUInt8()) << 16U) |
        (static_cast<std::size_t>(message[2].toUInt8()) << 8U) | static_cast<std::size_t>(message[3].toUInt8());
    if (bodyLength != message.size() - 4U) {
        throw err::ParameterFailure{"The TLS handshake message length is inconsistent."_el, "message"_el};
    }

    // RFC 8446 section 4.4.1: Transcript-Hash includes the complete encoded handshake message, including its header.
    _hasher.update(message);
}

auto Tls13Transcript::hash() const -> ByteBlock {
    // RFC 8446 section 4.4.1: snapshot the cumulative handshake context. Hasher copy-on-write leaves the live state
    // available for the next message and confines the finalized snapshot state to this call.
    auto snapshot = _hasher;
    return snapshot.finalize();
}

void Tls13Transcript::retry(const ConstByteSpan helloRetryRequest) {
    // RFC 8446 section 4.4.1: replace ClientHello1 with Handshake(message_hash, Hash(ClientHello1)).
    const auto firstHelloHash = hash();
    auto replacement = Tls13Transcript{algorithm()};
    const auto header =
        ByteArray<4U>{Byte{254U}, Byte{0U}, Byte{0U}, Byte{static_cast<uint8_t>(firstHelloHash.length().toSizeT())}};
    replacement._hasher.update(header.span());
    replacement._hasher.update(firstHelloHash.span());
    replacement.update(helloRetryRequest);
    // Install the complete replacement transcript before releasing the previous generation.
    _hasher = std::move(replacement._hasher);
}

void Tls13Transcript::secureErase() {
    // Keep transcript release visible because the hash worker contains message-dependent authentication state.
    _hasher.secureErase();
}

}
