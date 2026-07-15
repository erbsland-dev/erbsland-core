// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Byte order determines how multi-byte character encodings store data in memory.
/// UTF-16 and UTF-32 store characters as sequences of bytes, and the byte order
/// (little-endian or big-endian) affects how those sequences are laid out.
/// Little-endian stores the least significant byte first, while big-endian stores
/// the most significant byte first. This demo shows how the same text produces
/// different byte sequences depending on the chosen byte order.
void byteOrder() {
    // Encode a marine biology text in both UTF-16 byte orders.
    const auto oceanText = el::StringView{u8"🐋 Meerjungfrau 🌊"_el};
    el::io::printLine("Marine text: \"", oceanText, "\"\n");

    // Encode as UTF-16 little-endian (least significant byte first).
    auto bytes = el::StringEncoder{oceanText}.encode(el::StringEncoding::Utf16LittleEndian, el::StringBomMode::Reject);
    el::io::printLine("Encoded as UTF-16 little-endian:\n", el::ByteFormat::memoryDump(), bytes);

    // Encode as UTF-16 big-endian (most significant byte first).
    bytes = el::StringEncoder{oceanText}.encode(el::StringEncoding::Utf16BigEndian, el::StringBomMode::Reject);
    el::io::printLine("Encoded as UTF-16 big-endian:\n", el::ByteFormat::memoryDump(), bytes);
}

}
