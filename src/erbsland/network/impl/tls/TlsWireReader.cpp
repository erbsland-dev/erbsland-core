// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsWireReader.hpp"

#include "TlsProtocolError.hpp"

#include "../../../text/Literals.hpp"

namespace erbsland::network::impl {

using namespace text::literals;

auto TlsWireReader::readU8() -> uint8_t {
    require(1U);
    return _data[_offset++].toUInt8();
}

auto TlsWireReader::readU16() -> uint16_t {
    // RFC 8446 section 3: multi-byte integers use network byte order.
    require(2U);
    const auto result = static_cast<uint16_t>(
        (static_cast<uint16_t>(_data[_offset].toUInt8()) << 8U) | static_cast<uint16_t>(_data[_offset + 1U].toUInt8()));
    _offset += 2U;
    return result;
}

auto TlsWireReader::readU24() -> uint32_t {
    // RFC 8446 section 3: uint24 values are three network-order octets and fit exactly in the low 24 native bits.
    require(3U);
    const auto result = (static_cast<uint32_t>(_data[_offset].toUInt8()) << 16U) |
        (static_cast<uint32_t>(_data[_offset + 1U].toUInt8()) << 8U) |
        static_cast<uint32_t>(_data[_offset + 2U].toUInt8());
    _offset += 3U;
    return result;
}

auto TlsWireReader::readU32() -> uint32_t {
    // RFC 8446 section 3: uint32 values use network byte order.
    require(4U);
    const auto result = (static_cast<uint32_t>(_data[_offset].toUInt8()) << 24U) |
        (static_cast<uint32_t>(_data[_offset + 1U].toUInt8()) << 16U) |
        (static_cast<uint32_t>(_data[_offset + 2U].toUInt8()) << 8U) |
        static_cast<uint32_t>(_data[_offset + 3U].toUInt8());
    _offset += 4U;
    return result;
}

auto TlsWireReader::readBytes(const std::size_t length) -> mem::ConstByteSpan {
    require(length);
    const auto result = _data.subspan(_offset, length);
    _offset += length;
    return result;
}

auto TlsWireReader::readVector8() -> mem::ConstByteSpan {
    return readBytes(readU8());
}

auto TlsWireReader::readVector16() -> mem::ConstByteSpan {
    return readBytes(readU16());
}

auto TlsWireReader::readVector24() -> mem::ConstByteSpan {
    return readBytes(readU24());
}

void TlsWireReader::requireEnd() const {
    if (!isAtEnd()) {
        throw TlsProtocolError{TlsAlertDescription::DecodeError, "A TLS value contains trailing bytes."_el};
    }
}

void TlsWireReader::require(const std::size_t length) const {
    if (length > remaining()) {
        throw TlsProtocolError{TlsAlertDescription::DecodeError, "A TLS value is truncated."_el};
    }
}

}
