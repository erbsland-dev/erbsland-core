// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

#include <compare>

namespace demo {

/// Compare byte blocks lexicographically or without content-dependent exits.
///
/// The ordinary comparison operators are appropriate for ordering and general
/// equality checks. `isEqualConstTime()` inspects every byte for equal-length
/// inputs and is the safer content comparison for authentication-related data.
void comparingBlocks() {
    const auto novice = el::ByteBlock{el::Byte{1U}, el::Byte{3U}, el::Byte{5U}};
    const auto sameNovice = el::ByteBlockEditor{el::Byte{1U}, el::Byte{3U}, el::Byte{5U}};
    const auto adept = el::ByteBlock{el::Byte{1U}, el::Byte{4U}, el::Byte{1U}};

    // Use regular comparisons for equality and skill-tree ordering.
    const auto sameBuild = novice == sameNovice;
    const auto noviceComesFirst = (novice <=> adept) == std::strong_ordering::less;

    // Use constant-time content comparison for equal-length verification tokens.
    const auto matchingToken = novice.isEqualConstTime(sameNovice.span());
    const auto differentToken = novice.isEqualConstTime(adept);

    el::io::printLine("Build              : Apprentie des runes"_el);
    el::io::printLine("Same build         : "_el, el::BooleanFormat::yesNo(), sameBuild);
    el::io::printLine("Sorts before adept : "_el, el::BooleanFormat::yesNo(), noviceComesFirst);
    el::io::printLine("Matching token     : "_el, el::BooleanFormat::yesNo(), matchingToken);
    el::io::printLine("Different token    : "_el, el::BooleanFormat::yesNo(), differentToken);
}

}
