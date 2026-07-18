// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "SharedTools.hpp"

#include <DemoCommon.hpp>

namespace demo {

/// Get the first word in the given text with letters.
auto getFirstWordRange(const el::String &text) -> el::ByteRange {
    static const auto separator = el::CharSet::from(el::UnicodeCategoryGroup::Letter);
    const auto begin = text.findFirstOf(separator);
    if (begin.isNoIndex()) {
        return {};
    }
    auto end = text.findFirstNotOf(separator, begin);
    if (end.isNoIndex()) {
        end = text.indexAt(el::StringSide::Back);
    }
    return el::ByteRange{begin, end};
}

/// This demo demonstrates how a `String` can be constructed from various sources and how it behaves.
void constructionAndStorage() {
    // Constructing a `String` will just store a safe reference to the string literal.
    const auto textForRead = el::String{"🌲 Schwarzwald: kühle Morgenluft"_el};
    el::io::printLine("A string from a string literal: "_el, textForRead);
    printMemoryAndRangeInfo(textForRead);
    // Constructing a `StringEditor` will always create a copy of the passed string literal.
    const auto textForEdit = el::StringEditor{"🌴 Bali: hangatnya angin fajar"_el};
    el::io::printLine("A string from a literal: "_el, textForEdit);
    printMemoryAndRangeInfo(textForEdit);

    // `String`s are created implicitly from `StringEditor` objects.
    // The read-only string stores an owning reference to the editor's storage.
    // Even when the editor is destroyed, the string and the storage remain valid.
    const el::String stringFromString = textForEdit;
    // `String`s are also created implicitly from `StringLiteral` objects.
    // That makes them the primary choice for function parameters and to store strings.
    const el::String stringFromLiteral = "🌳 Forêt humide après la pluie"_el;

    // The strings look identical but carry different storage references.
    // From a user perspective, there is no difference in behavior.
    el::io::printLine("String from editor  : "_el, stringFromString);
    printMemoryAndRangeInfo(stringFromString);
    el::io::printLine("String from literal : "_el, stringFromLiteral);
    printMemoryAndRangeInfo(stringFromLiteral);

    // When sliced, strings keep the same storage reference but change their visible range.
    auto word = stringFromString.slice(getFirstWordRange(stringFromString));
    el::io::printLine("First word #1 : "_el, word);
    printMemoryAndRangeInfo(word);
    word = stringFromLiteral.slice(getFirstWordRange(stringFromLiteral));
    el::io::printLine("First word #2 : "_el, word);
    printMemoryAndRangeInfo(word);
}

}
