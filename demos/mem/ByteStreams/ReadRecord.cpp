// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

#include <cstdint>

namespace demo {

/// Parse observation fields while sharing a byte reader's cursor.
///
/// Passing a `ByteReader` by reference lets nested parsing functions consume
/// their own fields from one record without synchronizing separate offsets.
void readObservation(el::ByteReader &reader, uint16_t &count, uint8_t &confidence) {
    count = reader.readUInt16OrThrow();
    confidence = reader.readUInt8OrThrow();
}

/// Parse a short binary record with a local byte reader.
///
/// The local reader owns the overall operation. Helpers receive it by reference
/// when a format is divided into logical sections.
void readRecord() {
    const auto record = el::ByteBlock{el::Byte{0x4eU}, el::Byte{0x00U}, el::Byte{0x0cU}, el::Byte{94U}};
    auto reader = el::ByteReader{record};
    reader.setEndianness(el::Endianness::Big);

    // Parse the header locally, then delegate the observation fields.
    const auto marker = reader.readByteOrThrow();
    auto count = uint16_t{};
    auto confidence = uint8_t{};
    readObservation(reader, count, confidence);

    el::io::printLine("Waarneming        : Bosuil"_el);
    el::io::printLine("Marker            : "_el, marker.toUInt32());
    el::io::printLine("Calls / confidence: "_el, count, " / "_el, confidence);
}

}
