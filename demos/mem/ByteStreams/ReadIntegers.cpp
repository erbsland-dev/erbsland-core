// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

#include <cstdint>

namespace demo {

/// Read native and explicitly formatted integers from a byte stream.
///
/// Native-width helpers use the reader's byte order. An explicit
/// `ByteIntegerFormat` also validates the wire representation and target range.
void readIntegers() {
    const auto record =
        el::ByteBlock{el::Byte{0x00U}, el::Byte{0x0cU}, el::Byte{0x81U}, el::Byte{0x2cU}, el::Byte{94U}};
    auto reader = el::ByteReader{record};
    reader.setEndianness(el::Endianness::Big);

    // Read a fixed native value, a compact formatted value, and an 8-bit field.
    const auto calls = reader.readUInt16OrThrow();
    const auto flightSeconds = reader.readIntegerOrThrow<uint32_t>(el::ByteIntegerFormat::UnsignedVariableLength);
    const auto confidence = reader.readUInt8(0U);

    el::io::printLine("Roepjes gehoord   : "_el, calls);
    el::io::printLine("Flight seconds    : "_el, flightSeconds);
    el::io::printLine("Confidence        : "_el, confidence);
}

}
