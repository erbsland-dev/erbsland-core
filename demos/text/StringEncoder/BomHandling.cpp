// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// The `StringEncoder` class handles byte order marks (BOM) during encoding, giving you full control
/// over how multi-byte Unicode encodings represent their byte order. A BOM is a special marker placed
/// at the start of a byte stream that identifies both the encoding and the byte order.
///
/// Different encodings treat BOMs differently: UTF-8 rarely uses them, while UTF-16 and UTF-32
/// conventionally include one. The `StringBomMode` enum provides three modes — `Automatic` follows
/// convention, `Require` forces a BOM, and `Reject` forbids one entirely.
void bomHandling() {
    // Encode a nature observation in multiple encodings, each with a different BOM strategy.
    const auto observation = el::String{u8"🌲 Waldlichtung im Morgennebel 🌫️"_el};
    el::io::printLine("Beobachtung: \"", observation, "\"\n"_el);

    // `Automatic` follows encoding conventions: no BOM for UTF-8, BOM for UTF-16 and UTF-32.
    auto bytes = el::StringEncoder{observation}.encode(el::StringEncoding::Utf8, el::StringBomMode::Automatic);
    el::io::printLine("UTF-8 (automatic, no BOM):\n"_el, el::ByteFormat::memoryDump(), bytes);

    bytes = el::StringEncoder{observation}.encode(el::StringEncoding::Utf16LittleEndian, el::StringBomMode::Automatic);
    el::io::printLine("UTF-16 LE (automatic, with BOM):\n"_el, el::ByteFormat::memoryDump(), bytes);

    bytes = el::StringEncoder{observation}.encode(el::StringEncoding::Utf32, el::StringBomMode::Automatic);
    el::io::printLine("UTF-32 (automatic, with BOM):\n"_el, el::ByteFormat::memoryDump(), bytes);

    // Force a BOM even when the encoding convention does not use one.
    bytes = el::StringEncoder{observation}.encode(el::StringEncoding::Utf8, el::StringBomMode::Require);
    el::io::printLine("UTF-8 with forced BOM:\n"_el, el::ByteFormat::memoryDump(), bytes);

    // Explicitly suppress the BOM, even when the encoding normally includes one.
    bytes = el::StringEncoder{observation}.encode(el::StringEncoding::Utf32, el::StringBomMode::Reject);
    el::io::printLine("UTF-32 without BOM:\n"_el, el::ByteFormat::memoryDump(), bytes);
}

}
