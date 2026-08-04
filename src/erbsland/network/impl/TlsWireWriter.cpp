// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsWireWriter.hpp"

#include "../../err/ParameterError.hpp"
#include "../../mem/Byte.hpp"
#include "../../mem/Endianness.hpp"
#include "../../text/Literals.hpp"

#include <limits>

namespace erbsland::network::impl {

using namespace text::literals;

void TlsWireWriter::writeU8(const uint8_t value) {
    _data.append(mem::Byte{value});
}

void TlsWireWriter::writeU16(const uint16_t value) {
    // RFC 8446 section 3: multi-byte integers are encoded in network byte order.
    _data.appendInteger<uint16_t>(value, mem::Endianness::Big);
}

void TlsWireWriter::writeU24(const uint32_t value) {
    if (value > 0x00ffffffU) {
        throw err::ParameterError{"A TLS uint24 value exceeds 24 bits."_el, "value"_el};
    }
    // RFC 8446 section 3: uint24 is the low 24 bits in network byte order, with no native padding octet.
    writeU8(static_cast<uint8_t>(value >> 16U));
    writeU8(static_cast<uint8_t>(value >> 8U));
    writeU8(static_cast<uint8_t>(value));
}

void TlsWireWriter::writeBytes(const mem::ConstByteSpan value) {
    _data.append(value);
}

void TlsWireWriter::writeVector8(const mem::ConstByteSpan value) {
    if (value.size() > std::numeric_limits<uint8_t>::max()) {
        throw err::ParameterError{"A TLS uint8 vector exceeds 255 bytes."_el, "value"_el};
    }
    writeU8(static_cast<uint8_t>(value.size()));
    writeBytes(value);
}

void TlsWireWriter::writeVector16(const mem::ConstByteSpan value) {
    if (value.size() > std::numeric_limits<uint16_t>::max()) {
        throw err::ParameterError{"A TLS uint16 vector exceeds 65535 bytes."_el, "value"_el};
    }
    writeU16(static_cast<uint16_t>(value.size()));
    writeBytes(value);
}

void TlsWireWriter::writeVector24(const mem::ConstByteSpan value) {
    if (value.size() > 0x00ffffffU) {
        throw err::ParameterError{"A TLS uint24 vector exceeds 16777215 bytes."_el, "value"_el};
    }
    writeU24(static_cast<uint32_t>(value.size()));
    writeBytes(value);
}

auto TlsWireWriter::finish() -> mem::ByteBlock {
    auto result = mem::ByteBlock{_data};
    _data.reset();
    return result;
}

}
