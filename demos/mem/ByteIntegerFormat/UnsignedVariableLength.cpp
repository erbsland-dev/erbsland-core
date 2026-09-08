// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

#include <cstdint>

namespace demo {

/// Store unsigned values with the prefix-coded variable-length format.
///
/// Small values occupy fewer bytes, while the format still covers the complete
/// `uint64_t` range. The reader derives the encoded length from the leading byte,
/// so consecutive values need no separate size field.
void unsignedVariableLength() {
    auto writer = el::ByteWriter{};

    // Encode three skill-point totals of increasing size.
    writer.writeIntegerOrThrow<uint64_t>(42U, el::ByteIntegerFormat::UnsignedVariableLength)
        .writeIntegerOrThrow<uint64_t>(300U, el::ByteIntegerFormat::UnsignedVariableLength)
        .writeIntegerOrThrow<uint64_t>(0x123456U, el::ByteIntegerFormat::UnsignedVariableLength);
    const auto bytes = writer.toByteBlock();

    // Decode each self-delimiting value from the stream.
    auto reader = el::ByteReader{bytes};
    const auto novicePoints = reader.readIntegerOrThrow<uint64_t>(el::ByteIntegerFormat::UnsignedVariableLength);
    const auto adeptPoints = reader.readIntegerOrThrow<uint64_t>(el::ByteIntegerFormat::UnsignedVariableLength);
    const auto masteryPoints = reader.readIntegerOrThrow<uint64_t>(el::ByteIntegerFormat::UnsignedVariableLength);

    el::io::printLine("Skill tree         : Héritage du phénix"_el);
    el::io::printLine("Encoded totals     : "_el, el::ByteFormat::separated(), bytes);
    el::io::printLine("Novice points      : "_el, novicePoints);
    el::io::printLine("Adept points       : "_el, adeptPoints);
    el::io::printLine("Mastery points     : "_el, masteryPoints);
}

}
