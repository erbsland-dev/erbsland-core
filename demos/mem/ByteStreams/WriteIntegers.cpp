// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

#include <cstdint>

namespace demo {

/// Write native-width and explicitly formatted integers to a byte stream.
///
/// Native helpers make the field width visible in the call. Explicit formats
/// cover compact or non-standard wire representations and reject values that do
/// not fit.
void writeIntegers() {
    auto writer = el::ByteWriter{};
    writer.setEndianness(el::Endianness::Big);

    // Store a 16-bit count, a compact duration, and an 8-bit confidence value.
    writer.writeUInt16(12U)
        .writeIntegerOrThrow<uint32_t>(300U, el::ByteIntegerFormat::UnsignedVariableLength)
        .writeUInt8(94U);

    el::io::printLine("Integer byte order: Big endian"_el);
    el::io::printLine("Encoded fields    : "_el, el::ByteFormat::separated(), writer.toByteBlock());
}

}
