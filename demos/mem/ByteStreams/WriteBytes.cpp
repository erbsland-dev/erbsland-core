// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Append individual bytes and existing byte sequences to a stream.
///
/// `writeByte()` and `writeBytes()` advance the position and return the writer,
/// so adjacent fields can be expressed as one readable chain.
void writeBytes() {
    const auto sensorPayload = el::ByteBlock{el::Byte{0xa1U}, el::Byte{0xb2U}, el::Byte{0xc3U}};
    auto writer = el::ByteWriter{};

    // Write a marker, a byte count, and the already assembled payload.
    writer.writeByte(el::Byte{0x4eU})
        .writeByte(el::Byte{static_cast<uint8_t>(sensorPayload.length().toSizeT())})
        .writeBytes(sensorPayload);

    el::io::printLine("Nachtmeting       : Vleermuis"_el);
    el::io::printLine("Encoded record    : "_el, el::ByteFormat::separated(), writer.toByteBlock());
}

}
