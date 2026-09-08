// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

#include <cstdint>

namespace demo {

/// Add the fields of a nocturnal wildlife observation to a byte stream.
///
/// Passing a `ByteWriter` by reference lets several serialization functions
/// contribute to one record while sharing its position and byte order.
void writeObservation(el::ByteWriter &writer, const uint16_t count, const uint8_t confidence) {
    writer.writeUInt16(count).writeUInt8(confidence);
}

/// Build a short binary record with a local byte writer.
///
/// The local writer owns the overall operation. Helpers receive it by reference
/// when a format is divided into logical sections.
void buildRecord() {
    auto writer = el::ByteWriter{};
    writer.setEndianness(el::Endianness::Big);

    // Write the record header locally, then delegate the observation fields.
    writer.writeByte(el::Byte{0x4eU});
    writeObservation(writer, 12U, 94U);
    const auto record = writer.toByteBlock();

    el::io::printLine("Waarneming        : Bosuil"_el);
    el::io::printLine("Encoded record    : "_el, el::ByteFormat::separated(), record);
}

}
