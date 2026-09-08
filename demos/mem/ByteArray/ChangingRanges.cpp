// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Fill, overwrite, and combine ranges in a fixed-size `ByteArray`.
///
/// Range operations stay within the array and use as much of their source as
/// fits. Whole-array XOR validates equal lengths before changing anything, while
/// ranged XOR is a clamped operation for deliberately partial data.
void changingRanges() {
    auto waveSamples = el::ByteArray<8>{};

    // Establish a quiet baseline, then raise a three-sample pulse in the middle.
    waveSamples.fill(el::Byte{16U});
    waveSamples.fill(el::ByteRange{el::ByteIndex{2U}, el::ByteLength{3U}}, el::Byte{64U});

    // Copy complete or partial source waves into selected positions.
    waveSamples.overwrite(el::ByteArray{el::Byte{20U}, el::Byte{30U}}.span());
    waveSamples.overwrite(
        el::ByteIndex{5U}, el::ByteArray{el::Byte{80U}, el::Byte{96U}, el::Byte{112U}, el::Byte{128U}}.span());
    waveSamples.overwrite(
        el::ByteRange{el::ByteIndex{3U}, el::ByteLength{2U}},
        el::ByteArray{el::Byte{70U}, el::Byte{78U}, el::Byte{86U}}.span());

    // Apply a partial mask, then require an exact-size mask for the complete array.
    waveSamples.xorWith(
        el::ByteRange{el::ByteIndex{2U}, el::ByteLength{3U}},
        el::ByteArray{el::Byte{1U}, el::Byte{2U}, el::Byte{4U}}.span());
    const auto shortMaskAccepted = waveSamples.xorWith(el::ByteArray{el::Byte{255U}}.span());
    waveSamples.xorWithOrThrow(
        el::ByteArray{
            el::Byte{0U},
            el::Byte{0U},
            el::Byte{0U},
            el::Byte{0U},
            el::Byte{0U},
            el::Byte{0U},
            el::Byte{0U},
            el::Byte{0x0fU}}
            .span());

    el::io::printLine("Wave               : Havsvåg"_el);
    el::io::printLine("Combined samples   : "_el, el::ByteFormat::separated(), el::ByteBlock{waveSamples});
    el::io::printLine("Short mask accepted: "_el, el::BooleanFormat::yesNo(), shortMaskAccepted);
}

}
