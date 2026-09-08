// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Change selected bytes in a fixed-size `ByteArray`.
///
/// `set()` and `xorAt()` quietly ignore invalid indexes when absence is an
/// expected possibility. Their `OrThrow` counterparts make a valid index an
/// invariant and report a violation with an exception.
void changingBytes() {
    auto waveSamples = el::ByteArray{el::Byte{32U}, el::Byte{64U}, el::Byte{96U}, el::Byte{64U}, el::Byte{32U}};

    // Replace two samples whose positions are guaranteed by the waveform layout.
    waveSamples.set(el::ByteIndex{1U}, el::Byte{72U});
    waveSamples.setOrThrow(el::ByteIndex{3U}, el::Byte{72U});

    // Toggle flag bits directly at selected positions without a separate read.
    waveSamples.xorAt(el::ByteIndex{0U}, el::Byte{0x08U});
    waveSamples.xorAtOrThrow(el::ByteIndex{4U}, el::Byte{0x08U});

    // Tolerant writes leave the array unchanged when an index is unavailable.
    const auto beforeIgnoredWrites = waveSamples;
    waveSamples.set(el::ByteIndex{99U}, el::Byte{255U});
    waveSamples.xorAt(el::ByteIndex::noIndex(), el::Byte{255U});

    el::io::printLine("Wave               : Havsvåg"_el);
    el::io::printLine("Adjusted samples   : "_el, el::ByteFormat::separated(), el::ByteBlock{waveSamples});
    el::io::printLine("Invalid writes ignored: "_el, el::BooleanFormat::yesNo(), waveSamples == beforeIgnoredWrites);
}

}
