// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/all.hpp>

#include <cstdint>

namespace app::charset {

/// Prepared immutable inputs for one character-set scenario.
/// @notest{Validated by every character-set profiling sample.}
struct CharSetFixture {
    el::String caseName;                                       ///< Stable built-in input-case name.
    bool trackAllocations{};                                   ///< Whether allocation instrumentation is enabled.
    el::CharSet source;                                        ///< The primary prepared character set.
    el::CharSet other;                                         ///< The secondary prepared character set.
    el::Char character{U'x'};                                  ///< The character operand or lookup probe.
    el::CharRange range{U'd', U'q'};                           ///< The range operand.
    el::U8String u8Text;                                       ///< UTF-8 constructor or pattern input.
    el::U16String u16Text;                                     ///< UTF-16 pattern input.
    el::U32String u32Text;                                     ///< UTF-32 pattern input.
    el::util::Set<el::Char> characterSet;                      ///< Ordered character-set constructor input.
    el::util::List<el::Char> characterList;                    ///< Character-list constructor input.
    el::AsciiCategory asciiCategory{el::AsciiCategory::Digit}; ///< ASCII category factory input.
    el::UnicodeCategory unicodeCategory{el::UnicodeCategory::DecimalNumber}; ///< Unicode category factory input.
    el::UnicodeCategoryGroup unicodeGroup{el::UnicodeCategoryGroup::Letter}; ///< Unicode group factory input.
    std::uint64_t characterCount{};                                          ///< Primary input scalar-value count.
};

}
