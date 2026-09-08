// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

#include <cstdint>

namespace demo {

/// Read a compact classification record one bit at a time.
///
/// `BitReader` borrows a byte span and keeps the position of the next bit.
/// Boundary queries let a parser validate a field before consuming it, while
/// `advance()` and `setBitPosition()` provide safe cursor navigation.
void readClassification() {
    const auto record = el::ByteArray{el::Byte{0b10110110U}, el::Byte{0b11000000U}};
    auto reader = el::BitReader{record.span()};

    // Validate the fixed header, then consume its flags and three-bit class.
    const auto hasHeader = reader.canRead(8U);
    const auto potentiallyHazardous = reader.readBool();
    const auto confirmed = reader.readInteger<uint8_t>() != 0U;
    auto asteroidClass = static_cast<uint8_t>(reader.readInteger<uint8_t>() << 2U);
    asteroidClass |= static_cast<uint8_t>(reader.readInteger<uint8_t>() << 1U);
    asteroidClass |= reader.readInteger<uint8_t>();

    // Skip two reserved bits and read the final header flag.
    reader.advance(2U);
    const auto manuallyReviewed = reader.readBool();
    const auto payloadPosition = reader.bitPosition();

    // Revisit the payload and demonstrate that oversized positions clamp to the end.
    reader.setBitPosition(payloadPosition);
    const auto highConfidence = reader.readBool();
    reader.setBitPosition(100U);

    el::io::printLine("Klassifikation     : Jordnær asteroide"_el);
    el::io::printLine("Bits / header ready: "_el, reader.bitCount(), " / "_el, el::BooleanFormat::yesNo(), hasHeader);
    el::io::printLine(
        "Hazard / confirmed : "_el,
        el::BooleanFormat::yesNo(),
        potentiallyHazardous,
        " / "_el,
        el::BooleanFormat::yesNo(),
        confirmed);
    el::io::printLine("Class              : "_el, asteroidClass);
    el::io::printLine(
        "Reviewed / confidence: "_el,
        el::BooleanFormat::yesNo(),
        manuallyReviewed,
        " / "_el,
        el::BooleanFormat::yesNo(),
        highConfidence);
    el::io::printLine(
        "Remaining / at end : "_el, reader.remainingBitCount(), " / "_el, el::BooleanFormat::yesNo(), reader.isAtEnd());
}

}
