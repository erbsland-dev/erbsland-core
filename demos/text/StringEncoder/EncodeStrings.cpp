// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

/// The `StringEncoder` class converts text into a selected Unicode byte encoding.
/// It supports UTF-8, UTF-16, and UTF-32 output, optional byte order marks, and
/// explicit little-endian or big-endian byte order for encodings where this matters.
void encodeStrings() {
    // Encode a short Unicode text into UTF-32 little-endian bytes with a BOM.
    const auto observation = el::U8StringView{"Sternbild: Orion ✨"_el};
    const auto encodedObservation =
        el::StringEncoder{observation}.encode(el::StringEncoding::Utf32LittleEndian, el::StringBomMode::Require);

    el::io::printLine("Observation: \"", observation, "\"");
    el::io::printLine("Encoded as UTF-32 little-endian with BOM:");
    el::io::printLine(el::ByteFormat::memoryDump(), encodedObservation);
}
