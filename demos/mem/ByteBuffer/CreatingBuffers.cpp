// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Create independent, reusable byte buffers from common inputs.
///
/// A `ByteBuffer` owns mutable storage and copies both its construction input
/// and other buffers. It is a good fit for working memory that changes several
/// times before its contents become a stable application value.
void creatingBuffers() {
    // Start with empty, repeated, and explicitly listed forest measurements.
    const auto emptySurvey = el::ByteBuffer{};
    const auto clearings = el::ByteBuffer{el::ByteLength{4U}, el::Byte{0U}};
    const auto canopy = el::ByteBuffer{el::Byte{18U}, el::Byte{42U}, el::Byte{67U}};

    // Copy a borrowed span into a buffer with an independent lifetime.
    const auto soilSamples = el::ByteArray{el::Byte{7U}, el::Byte{11U}, el::Byte{15U}};
    auto copiedSamples = el::ByteBuffer{soilSamples.span()};

    // Copies are deep: changing one buffer never changes the other.
    auto independentCopy = copiedSamples;
    independentCopy.setOrThrow(el::ByteIndex{0U}, el::Byte{99U});

    el::io::printLine("Orman             : Karadeniz ormanı"_el);
    el::io::printLine("Empty survey      : "_el, el::BooleanFormat::yesNo(), emptySurvey.isEmpty());
    el::io::printLine(
        "Clearing samples  : "_el, el::ByteFormat::separated(), el::ByteBlock::fromSpan(clearings.span()));
    el::io::printLine("Canopy samples    : "_el, el::ByteFormat::separated(), el::ByteBlock::fromSpan(canopy.span()));
    el::io::printLine(
        "Original soil     : "_el, el::ByteFormat::separated(), el::ByteBlock::fromSpan(copiedSamples.span()));
    el::io::printLine(
        "Changed copy      : "_el, el::ByteFormat::separated(), el::ByteBlock::fromSpan(independentCopy.span()));
}

}
