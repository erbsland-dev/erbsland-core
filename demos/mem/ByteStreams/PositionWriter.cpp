// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Revisit fields while assembling a byte stream.
///
/// Writing at an existing position overwrites bytes. Setting a position beyond
/// the current length clamps it to the end, where subsequent writes append.
void positionWriter() {
    auto writer = el::ByteWriter{};

    // Leave a placeholder count, append events, then return to fill the count.
    writer.writeByte(el::Byte{}).writeByte(el::Byte{0x31U}).writeByte(el::Byte{0x32U});
    writer.setPosition(el::ByteIndex{0U});
    writer.writeByte(el::Byte{2U});
    writer.setPosition(el::ByteIndex{100U});
    writer.writeByte(el::Byte{0xffU});

    el::io::printLine("Final position     : "_el, writer.position().toSizeT());
    el::io::printLine("Record length      : "_el, writer.length().toSizeT());
    el::io::printLine("Encoded record     : "_el, el::ByteFormat::separated(), writer.toByteBlock());
}

}
