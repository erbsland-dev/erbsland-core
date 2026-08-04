// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Element helpers select, sample, and shuffle values without manual index math.
///
/// Use `selectElement()` for a single choice, `buildElementList()` for sampling
/// with replacement, `buildUniqueElementList()` for sampling without
/// replacement, and `shuffle()` when the complete order should change in place.
void elementSampling() {
    auto &random = el::application().random();
    auto palette = el::StringList{
        "błękit"_el,
        "zieleń"_el,
        "karmin"_el,
        "złoto"_el,
    };

    // Select one element directly or provide a fallback for empty input.
    const auto accent = random.selectElement(palette);
    const auto fallback = random.selectElement(el::StringList{}, el::String{"biel"_el});
    el::io::printLine("Accent color : "_el, accent);
    el::io::printLine("Fallback     : "_el, fallback);

    // Select an index when code needs to update a separate structure.
    const auto index = random.selectIndex(palette.count());
    el::io::printLine("Accent index : "_el, index.toSizeT());

    // Build repeated and unique samples from the same choices.
    const auto gradient = random.buildElementList(el::ItemCount{5U}, palette);
    const auto studySet = random.buildUniqueElementList(el::ItemCount{3U}, palette);
    el::io::printLine("Gradient     : "_el, gradient.join(", "_el));
    el::io::printLine("Study set    : "_el, studySet.join(", "_el));

    // Shuffle rearranges all elements while preserving the original values.
    random.shuffle(palette);
    el::io::printLine("Shuffled     : "_el, palette.join(", "_el));
}

}
