// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

#include <string>
#include <vector>

/// Erbsland Core uses a rich API approach to keep boilerplate to a minimum.
/// The common case stays short, readable, and maintainable.
/// More dangerous and error-prone code is intentionally more explicit.
/// This demo compares Erbsland Core code with equivalent standard-library code.
void stdVsCoreSplitAndJoin() {
    el::io::printLine("=== Splitting text into words ==="_el);

    // Erbsland Core: Split by a character set and ignore empty noise.
    constexpr auto elWordsText = "a, b, d; e; f g h, i,, j"_el;
    const auto noiseCharSet = el::CharSet::fromPattern(" ,;"_el);
    const auto coreWords = el::StringViewList::fromSplit(elWordsText, noiseCharSet);
    el::io::printLine("Core: "_el, coreWords.join("|"_el));

    // Standard library: Express the same behavior manually.
    constexpr auto stdWordsText = std::string_view{"a, b, d; e; f g h, i,, j"};
    auto isNoise = [](const char character) -> bool {
        return character == ' ' || character == ',' || character == ';';
    };
    std::vector<std::string> stdWords;
    auto wordStart = std::string_view::npos;
    for (std::size_t index = 0; index <= stdWordsText.size(); ++index) {
        if (index < stdWordsText.size() && !isNoise(stdWordsText[index])) {
            if (wordStart == std::string_view::npos) {
                wordStart = index;
            }
            continue;
        }

        if (wordStart != std::string_view::npos) {
            stdWords.emplace_back(stdWordsText.substr(wordStart, index - wordStart));
            wordStart = std::string_view::npos;
        }
    }
    std::string stdJoin;
    for (const auto &value : stdWords) {
        if (!stdJoin.empty()) {
            stdJoin += "|";
        }
        stdJoin += value;
    }
    el::io::printLine("std:  "_el, stdJoin);
}
