// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Read, update, and visit individual bytes in a working buffer.
///
/// Tolerant access ignores invalid writes or returns a chosen fallback. The
/// `OrThrow` variants are appropriate when the buffer layout guarantees that a
/// position exists, and `forEach()` handles complete traversal.
void accessingBytes() {
    auto treeHeights = el::ByteBuffer{el::Byte{12U}, el::Byte{19U}, el::Byte{27U}, el::Byte{35U}};

    // Treat one known position strictly and one optional position tolerantly.
    treeHeights.setOrThrow(el::ByteIndex{1U}, el::Byte{20U});
    treeHeights.set(el::ByteIndex{99U}, el::Byte{50U});
    treeHeights.xorAtOrThrow(el::ByteIndex{3U}, el::Byte{1U});
    const auto missingHeight = treeHeights.get(el::ByteIndex{99U}, el::Byte{255U});

    // Visit all trees to find the tallest recorded height.
    auto tallest = el::Byte{};
    const auto visitResult = treeHeights.forEach([&tallest](const el::Byte height) -> void {
        if (height > tallest) {
            tallest = height;
        }
    });

    el::io::printLine("Alan              : Yaşlı meşeler"_el);
    el::io::printLine(
        "Recorded heights  : "_el, el::ByteFormat::separated(), el::ByteBlock::fromSpan(treeHeights.span()));
    el::io::printLine("Missing fallback  : "_el, missingHeight.toUInt32());
    el::io::printLine("Tallest height    : "_el, tallest.toUInt32());
    el::io::printLine("Visited all bytes : "_el, el::BooleanFormat::yesNo(), visitResult == el::LoopResult::Success);
}

}
