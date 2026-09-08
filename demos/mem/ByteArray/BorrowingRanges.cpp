// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Borrow complete and clamped read-only spans from a `ByteArray`.
///
/// The complete span retains the array extent in its type. Ranged spans contain
/// only existing bytes, so an oversized length is clamped and an invalid start
/// produces an empty view. Every returned span remains tied to the array lifetime.
void borrowingRanges() {
    auto waveSamples =
        el::ByteArray{el::Byte{18U}, el::Byte{42U}, el::Byte{81U}, el::Byte{126U}, el::Byte{81U}, el::Byte{42U}};

    // Borrow the whole fixed extent and two ranges selected at runtime.
    const auto completeWave = waveSamples.span();
    const auto crest = waveSamples.span(el::ByteRange{el::ByteIndex{2U}, el::ByteLength{3U}});
    const auto clampedTail = waveSamples.span(el::ByteIndex{4U}, el::ByteLength{99U});
    const auto outside = waveSamples.span(el::ByteIndex{99U}, el::ByteLength{2U});
    static_assert(decltype(completeWave)::extent == 6U);

    // A span observes later in-place changes because it still views the array.
    waveSamples.setOrThrow(el::ByteIndex{2U}, el::Byte{88U});

    auto crestSum = uint32_t{};
    for (const auto sample : crest) {
        crestSum += sample.toUInt32();
    }

    el::io::printLine("Wave               : Havsvåg"_el);
    el::io::printLine("Complete length    : "_el, completeWave.size());
    el::io::printLine("Crest length       : "_el, crest.size());
    el::io::printLine("Clamped tail length: "_el, clampedTail.size());
    el::io::printLine("Outside is empty   : "_el, el::BooleanFormat::yesNo(), outside.empty());
    el::io::printLine("Updated crest sum  : "_el, crestSum);
}

}
