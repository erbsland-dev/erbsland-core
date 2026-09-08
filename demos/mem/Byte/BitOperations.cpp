// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Work with flags and compact fields in a single byte.
///
/// `Byte` supports the usual bitwise operators, shifts that fill with zero,
/// rotations that preserve every bit, and named helpers for both returning a
/// changed value and changing a byte in place. Masks can extract a field, while
/// `matches()` tests a field without converting the byte to an integer first.
void bitOperations() {
    const auto byteFormat =
        el::IntegerFormat::binary()
            .setFlags(
                el::IntegerFormatFlag::BasePrefix | el::IntegerFormatFlag::ZeroFill | el::IntegerFormatFlag::Separator)
            .setFieldWidth(el::CpLength{8U});
    const auto silkRouteMarker = el::Byte{0b10110110U};
    const auto transportMask = el::Byte{0b11000000U};
    const auto verifiedFlag = el::Byte{0b00100000U};

    // Combine, retain, toggle, and invert flags with the familiar operators.
    const auto combined = silkRouteMarker | verifiedFlag;
    const auto transport = silkRouteMarker & transportMask;
    const auto toggled = silkRouteMarker ^ verifiedFlag;
    const auto inverted = ~silkRouteMarker;

    auto editedFlags = silkRouteMarker;
    editedFlags |= verifiedFlag;
    editedFlags &= el::Byte{0b11110000U};
    editedFlags ^= el::Byte{0b00010000U};

    el::io::printLine("Route              : Zijderoute"_el);
    el::io::printLine("Original marker    : "_el, byteFormat, silkRouteMarker.toUInt32());
    el::io::printLine("Combined flags     : "_el, byteFormat, combined.toUInt32());
    el::io::printLine("Transport field    : "_el, byteFormat, transport.toUInt32());
    el::io::printLine("Toggled flags      : "_el, byteFormat, toggled.toUInt32());
    el::io::printLine("Inverted flags     : "_el, byteFormat, inverted.toUInt32());
    el::io::printLine("Edited flags       : "_el, byteFormat, editedFlags.toUInt32());

    // Shift bits when their old positions can be discarded.
    const auto shiftedByOperator = (silkRouteMarker << 1U) >> 2U;
    const auto shiftedByName = silkRouteMarker.shiftedLeft(1U).shiftedRight(2U);
    auto shiftedInPlace = silkRouteMarker;
    shiftedInPlace <<= 1U;
    shiftedInPlace >>= 2U;
    shiftedInPlace.shiftLeft(1U);
    shiftedInPlace.shiftRight(1U);

    el::io::printLine("Operator shifts    : "_el, byteFormat, shiftedByOperator.toUInt32());
    el::io::printLine("Named shifts       : "_el, byteFormat, shiftedByName.toUInt32());
    el::io::printLine("In-place shifts    : "_el, byteFormat, shiftedInPlace.toUInt32());

    // Rotate bits when every bit must remain in the byte.
    const auto rotatedLeft = silkRouteMarker.rotatedLeft(2);
    const auto rotatedRight = silkRouteMarker.rotatedRight(2);
    auto rotatedInPlace = silkRouteMarker;
    rotatedInPlace.rotateLeft(2);
    rotatedInPlace.rotateRight(2);

    el::io::printLine("Rotated left       : "_el, byteFormat, rotatedLeft.toUInt32());
    el::io::printLine("Rotated right      : "_el, byteFormat, rotatedRight.toUInt32());
    el::io::printLine("Rotation restored  : "_el, el::BooleanFormat::yesNo(), rotatedInPlace == silkRouteMarker);

    // Extract and test the encoded transport field with a mask.
    const auto maskedTransport = silkRouteMarker.masked(transportMask);
    const auto isSeaRoute = silkRouteMarker.matches(transportMask, el::Byte{0b10000000U});

    el::io::printLine("Masked transport   : "_el, byteFormat, maskedTransport.toUInt32());
    el::io::printLine("Sea route          : "_el, el::BooleanFormat::yesNo(), isSeaRoute);
}

}
