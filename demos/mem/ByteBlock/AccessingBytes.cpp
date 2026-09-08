// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Read individual bytes or visit a complete block.
///
/// Tolerant access returns a chosen fallback for an invalid index. Strict access
/// throws when a required byte is absent, and `forEach()` visits every visible
/// byte with an optional byte index.
void accessingBytes() {
    const auto skillCosts = el::ByteBlock{el::Byte{3U}, el::Byte{5U}, el::Byte{8U}, el::Byte{13U}};

    // Read one optional cost and one position guaranteed by the skill-tree layout.
    const auto missingCost = skillCosts.get(el::ByteIndex{20U}, el::Byte{255U});
    const auto masteryCost = skillCosts.getOrThrow(el::ByteIndex{3U});

    // Visit every cost together with its index to calculate a total.
    auto totalCost = uint32_t{};
    const auto visitResult = skillCosts.forEach(
        [&totalCost](const el::Byte cost, const el::ByteIndex) -> void { totalCost += cost.toUInt32(); });

    el::io::printLine("Skill tree         : Sentier du lynx"_el);
    el::io::printLine("End index          : "_el, skillCosts.endIndex().toSizeT());
    el::io::printLine("Missing fallback   : "_el, missingCost.toUInt32());
    el::io::printLine("Mastery cost       : "_el, masteryCost.toUInt32());
    el::io::printLine("Total cost         : "_el, totalCost);
    el::io::printLine("Visited all bytes  : "_el, el::BooleanFormat::yesNo(), visitResult == el::LoopResult::Success);
}

}
