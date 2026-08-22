// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsRecordStream.hpp"

#include "TlsAlertDescription.hpp"
#include "TlsProtocolError.hpp"

#include "../../../mem/Endianness.hpp"
#include "../../../text/Literals.hpp"
#include "../../../unit/ByteIndex.hpp"
#include "../../../unit/ByteRange.hpp"

namespace erbsland::network::impl {

using namespace text::literals;

void TlsRecordStream::append(const mem::ConstByteSpan data) {
    // Enforce the configured aggregate bound before extending storage with attacker-controlled stream bytes.
    if (data.size() > _bufferLimit.toSizeT() || _buffer.length().toSizeT() > _bufferLimit.toSizeT() - data.size()) {
        throw TlsProtocolError{TlsAlertDescription::RecordOverflow, "The TLS receive buffer limit was exceeded."_el};
    }
    _buffer.append(data);
}

auto TlsRecordStream::next() -> std::optional<mem::ByteBlock> {
    if (_buffer.length().toSizeT() < cHeaderLength) {
        return {};
    }

    // RFC 8446 sections 5.1 and 5.2: fragment length is the network-order uint16 in the exact five-byte header.
    const auto fragmentLength = _buffer.getIntegerOrThrow<uint16_t>(unit::ByteIndex{3U}, mem::Endianness::Big);
    if (fragmentLength > cMaximumFragmentLength) {
        throw TlsProtocolError{TlsAlertDescription::RecordOverflow, "The TLS record fragment exceeds 2^14 + 256."_el};
    }
    const auto recordLength = cHeaderLength + static_cast<std::size_t>(fragmentLength);
    if (_buffer.length().toSizeT() < recordLength) {
        return {};
    }

    // Copy the complete record before removing its storage from the incremental buffer.
    auto result = mem::ByteBlock::fromSpan(_buffer.span().first(recordLength));
    _buffer.remove(unit::ByteRange{unit::ByteIndex::zero(), unit::ByteLength{recordLength}});
    return result;
}

void TlsRecordStream::clear() noexcept {
    _buffer.reset();
}

}
