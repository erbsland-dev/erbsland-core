// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

#include <cstdint>

namespace demo {

/// Encode and decode integers with an explicit fixed-width wire format.
///
/// A `ByteIntegerFormat` selects signedness and byte width independently from
/// the C++ value type. Multi-byte fixed formats use the byte order configured on
/// the writer and reader.
void fixedFormats() {
    auto writer = el::ByteWriter{};
    writer.setEndianness(el::Endianness::Big);

    // Encode a 24-bit unsigned skill identifier and a 32-bit signed modifier.
    writer.writeIntegerOrThrow<uint32_t>(0x123456U, el::ByteIntegerFormat::UnsignedFixed24Bit)
        .writeIntegerOrThrow<int32_t>(-300, el::ByteIntegerFormat::SignedFixed32Bit);
    const auto bytes = writer.toByteBlock();

    // Decode the fields with the same formats and byte order.
    auto reader = el::ByteReader{bytes};
    reader.setEndianness(el::Endianness::Big);
    const auto skillId = reader.readIntegerOrThrow<uint32_t>(el::ByteIntegerFormat::UnsignedFixed24Bit);
    const auto modifier = reader.readIntegerOrThrow<int32_t>(el::ByteIntegerFormat::SignedFixed32Bit);
    const auto idFormat = el::ByteIntegerFormat{el::ByteIntegerFormat::UnsignedFixed24Bit};

    el::io::printLine("Skill              : Flèche d'aurore"_el);
    el::io::printLine("Encoded fields     : "_el, el::ByteFormat::separated(), bytes);
    el::io::printLine("Skill identifier   : "_el, skillId);
    el::io::printLine("Signed modifier    : "_el, modifier);
    el::io::printLine("Identifier bytes   : "_el, idFormat.byteCount().toSizeT());
    el::io::printLine("Identifier signed  : "_el, el::BooleanFormat::yesNo(), idFormat.isSigned());
}

}
