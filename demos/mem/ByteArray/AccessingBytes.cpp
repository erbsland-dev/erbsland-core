// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Inspect individual bytes in a fixed-size `ByteArray`.
///
/// The array reports its compile-time extent through byte-aware length and index
/// values. Tolerant access returns a fallback for an invalid index, strict access
/// throws, and `forEach()` can provide both each byte and its index.
void accessingBytes() {
    const auto waveSamples = el::ByteArray{el::Byte{24U}, el::Byte{63U}, el::Byte{112U}, el::Byte{63U}, el::Byte{24U}};

    // Inspect the fixed extent and read known sample positions.
    const auto firstSample = waveSamples.get(el::ByteIndex{0U});
    const auto crestSample = waveSamples.getOrThrow(el::ByteIndex{2U});

    // Choose an explicit fallback when an index may come from outside the array.
    const auto unavailableSample = waveSamples.get(el::ByteIndex{99U}, el::Byte{255U});

    // Visit every sample together with its index to locate the wave crest.
    auto peak = el::Byte{};
    auto peakIndex = el::ByteIndex::zero();
    const auto iterationResult = waveSamples.forEach([&](const el::Byte sample, const el::ByteIndex index) -> void {
        if (sample > peak) {
            peak = sample;
            peakIndex = index;
        }
    });

    el::io::printLine("Wave               : Havsvåg"_el);
    el::io::printLine("Sample count       : "_el, waveSamples.length().toSizeT());
    el::io::printLine("End index          : "_el, waveSamples.endIndex().toSizeT());
    el::io::printLine("Array is empty     : "_el, el::BooleanFormat::yesNo(), waveSamples.isEmpty());
    el::io::printLine("First sample       : "_el, firstSample.toUInt32());
    el::io::printLine("Crest sample       : "_el, crestSample.toUInt32());
    el::io::printLine("Missing fallback   : "_el, unavailableSample.toUInt32());
    el::io::printLine("Peak index         : "_el, peakIndex.toSizeT());
    el::io::printLine(
        "Visited all samples : "_el, el::BooleanFormat::yesNo(), iterationResult == el::LoopResult::Success);
}

}
