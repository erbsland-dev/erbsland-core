// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsWireWriter.hpp"

#include "../../../err/ParameterFailure.hpp"
#include "../../../mem/Byte.hpp"
#include "../../../mem/Endianness.hpp"
#include "../../../text/Literals.hpp"

#include <limits>

namespace erbsland::network::impl {

using namespace err;
using namespace mem;
using namespace text::literals;

void TlsWireWriter::writeU8(const uint8_t value) {
    _data.append(Byte{value});
}

void TlsWireWriter::writeU16(const uint16_t value) {
    // RFC 8446 section 3: multi-byte integers are encoded in network byte order.
    _data.appendInteger<uint16_t>(value, Endianness::Big);
}

void TlsWireWriter::writeU24(const uint32_t value) {
    if (value > 0x00ffffffU) {
        throw ParameterFailure{"A TLS uint24 value exceeds 24 bits."_el, "value"_el};
    }
    // RFC 8446 section 3: uint24 is the low 24 bits in network byte order, with no native padding octet.
    writeU8(static_cast<uint8_t>(value >> 16U));
    writeU8(static_cast<uint8_t>(value >> 8U));
    writeU8(static_cast<uint8_t>(value));
}

void TlsWireWriter::writeBytes(const ConstByteSpan value) {
    _data.append(value);
}

void TlsWireWriter::writeVector8(const ConstByteSpan value) {
    if (value.size() > std::numeric_limits<uint8_t>::max()) {
        throw ParameterFailure{"A TLS uint8 vector exceeds 255 bytes."_el, "value"_el};
    }
    writeU8(static_cast<uint8_t>(value.size()));
    writeBytes(value);
}

void TlsWireWriter::writeVector16(const ConstByteSpan value) {
    if (value.size() > std::numeric_limits<uint16_t>::max()) {
        throw ParameterFailure{"A TLS uint16 vector exceeds 65535 bytes."_el, "value"_el};
    }
    writeU16(static_cast<uint16_t>(value.size()));
    writeBytes(value);
}

void TlsWireWriter::writeVector24(const ConstByteSpan value) {
    if (value.size() > 0x00ffffffU) {
        throw ParameterFailure{"A TLS uint24 vector exceeds 16777215 bytes."_el, "value"_el};
    }
    writeU24(static_cast<uint32_t>(value.size()));
    writeBytes(value);
}

auto TlsWireWriter::finish() -> ByteBlock {
    auto result = ByteBlock{_data};
    _data.reset();
    return result;
}

}
