// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Tls13Transcript.hpp"

#include "../../../err/ParameterError.hpp"
#include "../../../text/Literals.hpp"

namespace erbsland::cryptology::impl {

using namespace text::literals;

Tls13Transcript::Tls13Transcript(const HashAlgorithm algorithm) : _hasher{algorithm} {
}

Tls13Transcript::~Tls13Transcript() {
    secureErase();
}

void Tls13Transcript::update(const mem::ConstByteSpan message) {
    // RFC 8446 section 4: every Handshake value has msg_type || uint24 length || body. Validate this framing before
    // adding bytes so a truncated or coalesced caller value cannot silently create a different transcript.
    if (message.size() < 4U) {
        throw err::ParameterError{"A complete TLS handshake header is required."_el, "message"_el};
    }
    const auto bodyLength = (static_cast<std::size_t>(message[1].toUInt8()) << 16U) |
        (static_cast<std::size_t>(message[2].toUInt8()) << 8U) | static_cast<std::size_t>(message[3].toUInt8());
    if (bodyLength != message.size() - 4U) {
        throw err::ParameterError{"The TLS handshake message length is inconsistent."_el, "message"_el};
    }

    // RFC 8446 section 4.4.1: Transcript-Hash includes the complete encoded handshake message, including its header.
    _hasher.update(message);
}

auto Tls13Transcript::hash() const -> mem::ByteBlock {
    // RFC 8446 section 4.4.1: snapshot the cumulative handshake context. Hasher copy-on-write leaves the live state
    // available for the next message and confines the finalized snapshot state to this call.
    auto snapshot = _hasher;
    return snapshot.finalize();
}

void Tls13Transcript::secureErase() {
    // Keep transcript release visible because the hash worker contains message-dependent authentication state.
    _hasher.secureErase();
}

}
