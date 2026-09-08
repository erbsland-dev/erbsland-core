// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

#include <cstdint>

namespace demo {

/// Encode canonical unsigned base-128 integers.
///
/// Each byte contributes seven payload bits. The high bit marks every byte that
/// is followed by another group, and the most-significant group comes first.
/// This representation is useful for ASN.1 identifier components.
void unsignedBase128() {
    auto writer = el::ByteWriter{};

    // Encode values around the one-byte and two-byte boundaries.
    writer.writeIntegerOrThrow<uint64_t>(127U, el::ByteIntegerFormat::UnsignedBase128)
        .writeIntegerOrThrow<uint64_t>(128U, el::ByteIntegerFormat::UnsignedBase128)
        .writeIntegerOrThrow<uint64_t>(16384U, el::ByteIntegerFormat::UnsignedBase128);
    const auto bytes = writer.toByteBlock();

    // Decode the canonical groups back into native values.
    auto reader = el::ByteReader{bytes};
    const auto firstNode = reader.readIntegerOrThrow<uint64_t>(el::ByteIntegerFormat::UnsignedBase128);
    const auto secondNode = reader.readIntegerOrThrow<uint64_t>(el::ByteIntegerFormat::UnsignedBase128);
    const auto thirdNode = reader.readIntegerOrThrow<uint64_t>(el::ByteIntegerFormat::UnsignedBase128);

    el::io::printLine("Skill route        : Constellation du mage"_el);
    el::io::printLine("Encoded nodes      : "_el, el::ByteFormat::separated(), bytes);
    el::io::printLine("First node         : "_el, firstNode);
    el::io::printLine("Second node        : "_el, secondNode);
    el::io::printLine("Third node         : "_el, thirdNode);
}

}
