// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Choose between shared slices and independent byte blocks.
///
/// `slice()` creates a cheap read-only view that keeps the complete shared
/// allocation alive. `copy()` and `kept()` allocate only the visible bytes, which
/// is useful when a small result must outlive a much larger source.
void slicingBlocks() {
    const auto completeTree = el::ByteBlock{
        el::Byte{1U}, el::Byte{2U}, el::Byte{3U}, el::Byte{5U}, el::Byte{8U}, el::Byte{13U}, el::Byte{21U}};
    const auto branchRange = el::ByteRange{el::ByteIndex{2U}, el::ByteLength{3U}};

    // Share the original allocation when the source and branch have similar lifetimes.
    const auto sharedBranch = completeTree.slice(branchRange);

    // Keep only the selected bytes when the branch will be stored independently.
    const auto compactBranch = completeTree.kept(branchRange);
    const auto explicitCopy = sharedBranch.copy();

    el::io::printLine("Skill tree         : Arbre du veilleur"_el);
    el::io::printLine("Complete tree      : "_el, el::ByteFormat::separated(), completeTree);
    el::io::printLine("Shared branch      : "_el, el::ByteFormat::separated(), sharedBranch);
    el::io::printLine("Compact branch     : "_el, el::ByteFormat::separated(), compactBranch);
    el::io::printLine("Explicit copy      : "_el, el::ByteFormat::separated(), explicitCopy);
}

}
