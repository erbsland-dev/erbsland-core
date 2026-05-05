// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "SharedTools.hpp"

#include <DemoCommon.hpp>

/// Get the first word in the given text with letters.
auto getFirstWordRange(const el::StringView &text) -> el::ByteRange {
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

/// This demo demonstrates how a `StringView` can be constructed from various sources and how it behaves.
void constructionAndStorage() {
    // Constructing a `StringView` will just store a safe reference to the string literal.
    const auto textForRead = el::StringView{"🌲 Schwarzwald: kühle Morgenluft"_el};
    el::io::printLine("A view from a string literal: "_el, textForRead);
    printMemoryAndRangeInfo(textForRead);
    // Constructing a `String` will always create a copy of the passed string literal.
    const auto textForEdit = el::String{"🌴 Bali: hangatnya angin fajar"_el};
    el::io::printLine("A string from a literal: "_el, textForEdit);
    printMemoryAndRangeInfo(textForEdit);

    // `StringView`s are created implicitly from `String` objects.
    // The view stores an owning reference to the string's storage.
    // Even when the string is destroyed, the view and the storage remain valid.
    const el::StringView viewFromString = textForEdit;
    // `StringView`s are also created implicitly from `StringLiteral` objects.
    // That makes them the primary choice for function parameters and to store strings.
    const el::StringView viewFromLiteral = "🌳 Forêt humide après la pluie"_el;

    // The views look identical but carry different storage references.
    // From a user perspective, there is no difference in behavior.
    el::io::printLine("View from string  : "_el, viewFromString);
    printMemoryAndRangeInfo(viewFromString);
    el::io::printLine("View from literal : "_el, viewFromLiteral);
    printMemoryAndRangeInfo(viewFromString);

    // When sliced, views keep the same storage reference but change the range they view from it.
    auto word = viewFromString.slice(getFirstWordRange(viewFromString));
    el::io::printLine("First word #1 : "_el, word);
    printMemoryAndRangeInfo(word);
    word = viewFromLiteral.slice(getFirstWordRange(viewFromLiteral));
    el::io::printLine("First word #2 : "_el, word);
    printMemoryAndRangeInfo(word);
}
