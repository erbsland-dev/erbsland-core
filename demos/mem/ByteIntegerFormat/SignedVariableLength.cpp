// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

#include <cstdint>

namespace demo {

/// Store signed values compactly with ZigZag mapping.
///
/// The signed variable-length format maps values around zero to small unsigned
/// numbers before applying prefix coding. Small positive and negative modifiers
/// therefore require similarly compact representations.
void signedVariableLength() {
    auto writer = el::ByteWriter{};

    // Encode nearby positive and negative skill modifiers, then a larger penalty.
    writer.writeIntegerOrThrow<int64_t>(-1, el::ByteIntegerFormat::SignedVariableLength)
        .writeIntegerOrThrow<int64_t>(1, el::ByteIntegerFormat::SignedVariableLength)
        .writeIntegerOrThrow<int64_t>(-300, el::ByteIntegerFormat::SignedVariableLength);
    const auto bytes = writer.toByteBlock();

    // Read the original signed values without handling the ZigZag mapping manually.
    auto reader = el::ByteReader{bytes};
    const auto shadowPenalty = reader.readIntegerOrThrow<int64_t>(el::ByteIntegerFormat::SignedVariableLength);
    const auto lightBonus = reader.readIntegerOrThrow<int64_t>(el::ByteIntegerFormat::SignedVariableLength);
    const auto masteryPenalty = reader.readIntegerOrThrow<int64_t>(el::ByteIntegerFormat::SignedVariableLength);

    el::io::printLine("Skill pair         : Ombre et lumière"_el);
    el::io::printLine("Encoded modifiers  : "_el, el::ByteFormat::separated(), bytes);
    el::io::printLine("Shadow modifier    : "_el, shadowPenalty);
    el::io::printLine("Light modifier     : "_el, lightBonus);
    el::io::printLine("Mastery modifier   : "_el, masteryPenalty);
}

}
