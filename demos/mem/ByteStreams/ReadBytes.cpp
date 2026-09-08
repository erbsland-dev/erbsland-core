// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Inspect and consume individual bytes and complete byte ranges.
///
/// Peek operations do not move the reader. Tolerant reads return a fallback or
/// no value when data is unavailable; `OrThrow` reads express required fields.
void readBytes() {
    const auto record =
        el::ByteBlock{el::Byte{0x4eU}, el::Byte{0x03U}, el::Byte{0xa1U}, el::Byte{0xb2U}, el::Byte{0xc3U}};
    auto reader = el::ByteReader{record};

    // Inspect the record marker, then consume the marker and payload length.
    const auto marker = reader.peekByteOrThrow();
    reader.advance(1U);
    const auto payloadLength = reader.readByteOrThrow().toUInt8();

    // Read the complete payload atomically.
    const auto payload = reader.readBytes(el::ByteLength{payloadLength});
    const auto extra = reader.readBytes(el::ByteLength{1U});
    el::io::printLine("Marker            : "_el, marker.toUInt32());
    el::io::printLine("Payload           : "_el, el::ByteFormat::separated(), *payload);
    el::io::printLine("Extra byte        : "_el, el::BooleanFormat::yesNo(), extra.has_value());
}

}
