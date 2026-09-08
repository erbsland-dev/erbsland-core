// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Shift or rotate a complete `ByteArray` and each byte independently.
///
/// Whole-array operations treat index zero as the most-significant byte and let
/// bits cross byte boundaries. The `eachByte...()` family keeps every byte as a
/// separate value. Both families offer result-returning and in-place forms.
void movingBits() {
    const auto shiftWave = el::ByteArray{el::Byte{0x81U}, el::Byte{0x80U}};
    const auto rotationWave = el::ByteArray{el::Byte{0x81U}, el::Byte{0x40U}};

    // Shift one packed 16-bit pattern and two independent eight-bit samples.
    const auto wholeShiftedLeft = shiftWave.shiftedLeft(1U);
    const auto wholeShiftedRight = shiftWave.shiftedRight(1U);
    const auto bytesShiftedLeft = shiftWave.eachByteShiftedLeft(1U);
    const auto bytesShiftedRight = shiftWave.eachByteShiftedRight(1U);

    // Rotate with and without crossing the boundary between the two bytes.
    const auto wholeRotatedLeft = rotationWave.rotatedLeft(1);
    const auto wholeRotatedRight = rotationWave.rotatedRight(1);
    const auto bytesRotatedLeft = rotationWave.eachByteRotatedLeft(1);
    const auto bytesRotatedRight = rotationWave.eachByteRotatedRight(1);

    // The mutating forms produce the same values in their original arrays.
    auto changedWholeLeft = shiftWave;
    auto changedWholeRight = shiftWave;
    auto changedBytesLeft = shiftWave;
    auto changedBytesRight = shiftWave;
    auto changedWholeRotateLeft = rotationWave;
    auto changedWholeRotateRight = rotationWave;
    auto changedBytesRotateLeft = rotationWave;
    auto changedBytesRotateRight = rotationWave;
    changedWholeLeft.shiftLeft(1U);
    changedWholeRight.shiftRight(1U);
    changedBytesLeft.shiftEachByteLeft(1U);
    changedBytesRight.shiftEachByteRight(1U);
    changedWholeRotateLeft.rotateLeft(1);
    changedWholeRotateRight.rotateRight(1);
    changedBytesRotateLeft.rotateEachByteLeft(1);
    changedBytesRotateRight.rotateEachByteRight(1);

    const auto inPlaceFormsAgree = changedWholeLeft == wholeShiftedLeft && changedWholeRight == wholeShiftedRight &&
        changedBytesLeft == bytesShiftedLeft && changedBytesRight == bytesShiftedRight &&
        changedWholeRotateLeft == wholeRotatedLeft && changedWholeRotateRight == wholeRotatedRight &&
        changedBytesRotateLeft == bytesRotatedLeft && changedBytesRotateRight == bytesRotatedRight;

    el::io::printLine("Wave               : Havsvåg"_el);
    el::io::printLine("Whole shift left   : "_el, el::ByteFormat::separated(), el::ByteBlock{wholeShiftedLeft});
    el::io::printLine("Each byte left     : "_el, el::ByteFormat::separated(), el::ByteBlock{bytesShiftedLeft});
    el::io::printLine("Whole shift right  : "_el, el::ByteFormat::separated(), el::ByteBlock{wholeShiftedRight});
    el::io::printLine("Each byte right    : "_el, el::ByteFormat::separated(), el::ByteBlock{bytesShiftedRight});
    el::io::printLine("Whole rotate left  : "_el, el::ByteFormat::separated(), el::ByteBlock{wholeRotatedLeft});
    el::io::printLine("Each byte left rot.: "_el, el::ByteFormat::separated(), el::ByteBlock{bytesRotatedLeft});
    el::io::printLine("Whole rotate right : "_el, el::ByteFormat::separated(), el::ByteBlock{wholeRotatedRight});
    el::io::printLine("Each byte right rot: "_el, el::ByteFormat::separated(), el::ByteBlock{bytesRotatedRight});
    el::io::printLine("In-place forms agree: "_el, el::BooleanFormat::yesNo(), inPlaceFormsAgree);
}

}
