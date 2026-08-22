// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsHandshakeStream.hpp"

#include "TlsAlertDescription.hpp"
#include "TlsProtocolError.hpp"

#include "../../../text/Literals.hpp"
#include "../../../unit/ByteIndex.hpp"
#include "../../../unit/ByteRange.hpp"

namespace erbsland::network::impl {

using namespace text::literals;

void TlsHandshakeStream::append(const mem::ConstByteSpan data) {
    // Bound incomplete handshake storage before allocation. A complete message is removed as soon as next() sees it.
    constexpr auto cAggregateLimit = cHeaderLength + cMaximumBodyLength;
    if (data.size() > cAggregateLimit || _buffer.length().toSizeT() > cAggregateLimit - data.size()) {
        throw TlsProtocolError{TlsAlertDescription::DecodeError, "The TLS handshake buffer limit was exceeded."_el};
    }
    _buffer.append(data);
}

auto TlsHandshakeStream::next() -> std::optional<mem::ByteBlock> {
    if (_buffer.length().toSizeT() < cHeaderLength) {
        return {};
    }

    // RFC 8446 section 4: the handshake body length is a network-order uint24 following msg_type.
    const auto bytes = _buffer.span();
    const auto bodyLength = (static_cast<std::size_t>(bytes[1].toUInt8()) << 16U) |
        (static_cast<std::size_t>(bytes[2].toUInt8()) << 8U) | static_cast<std::size_t>(bytes[3].toUInt8());
    if (bodyLength > cMaximumBodyLength) {
        throw TlsProtocolError{TlsAlertDescription::DecodeError, "A TLS handshake message exceeds one MiB."_el};
    }
    const auto messageLength = cHeaderLength + bodyLength;
    if (_buffer.length().toSizeT() < messageLength) {
        return {};
    }

    auto result = mem::ByteBlock::fromSpan(bytes.first(messageLength));
    _buffer.remove(unit::ByteRange{unit::ByteIndex::zero(), unit::ByteLength{messageLength}});
    return result;
}

void TlsHandshakeStream::clear() noexcept {
    _buffer.secureErase();
    _buffer.reset();
}

}
