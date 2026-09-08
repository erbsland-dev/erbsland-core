// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Convert between a byte and character, integer, or `std::byte` values.
///
/// Named conversions document the intended representation at each boundary.
/// The `fromCropped...()` factories make truncation explicit when a wider
/// integer contributes only its lowest eight bits.
void conversions() {
    const auto byteFormat =
        el::IntegerFormat::binary()
            .setFlags(
                el::IntegerFormatFlag::BasePrefix | el::IntegerFormatFlag::ZeroFill | el::IntegerFormatFlag::Separator)
            .setFieldWidth(el::CpLength{8U});

    // Create bytes from values read from a compact route archive.
    const auto direction = el::Byte::fromChar('E');
    const auto waypoint = el::Byte::fromUInt8(42U);
    const auto legacyMarker = el::Byte::fromCroppedUInt16(0x12abU);
    const auto mapMarker = el::Byte::fromCroppedUInt32(0x123456cdU);
    const auto catalogMarker = el::Byte::fromCroppedUInt64(0x123456789abcdef0ULL);

    el::io::printLine("Route              : Zijderoute"_el);
    el::io::printLine("Direction          : "_el, direction.toChar());
    el::io::printLine("Waypoint           : "_el, byteFormat, waypoint.toUInt32());
    el::io::printLine("Legacy marker      : "_el, byteFormat, legacyMarker.toUInt32());
    el::io::printLine("Map marker         : "_el, byteFormat, mapMarker.toUInt32());
    el::io::printLine("Catalog marker     : "_el, byteFormat, catalogMarker.toUInt32());

    // Convert one byte to the width expected by each receiving API.
    const auto standardByte = waypoint.toStdByte();
    const auto count8 = waypoint.toUInt8();
    const auto count16 = waypoint.toUInt16();
    const auto count32 = waypoint.toUInt32();
    const auto count64 = waypoint.toUInt64();

    el::io::printLine("As std::byte       : "_el, byteFormat, std::to_integer<uint32_t>(standardByte));
    el::io::printLine("As uint8_t         : "_el, byteFormat, count8);
    el::io::printLine("As uint16_t        : "_el, byteFormat, count16);
    el::io::printLine("As uint32_t        : "_el, byteFormat, count32);
    el::io::printLine("As uint64_t        : "_el, byteFormat, count64);
}

}
