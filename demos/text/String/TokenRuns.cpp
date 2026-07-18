// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Combining findFirstOf and findFirstNotOf extracts runs of matching characters.
///
/// Define the characters that belong to a token, then search for the next
/// matching character and the next non-matching character. Everything outside
/// the token set acts as a separator, so punctuation, spaces, and symbols do not
/// need separate handling.
void tokenRuns() {
    static const auto tokenCharacters = el::CharSet::from(el::UnicodeCategoryGroup::Letter) |
        el::CharSet::from(el::UnicodeCategory::DecimalNumber) | el::CharSet{U'-'};

    const auto route = el::String{"Rutt: Havsörn-7 går mot djupzon Ålvik; prov=A12; temp=4°C"_el};

    el::io::printLine("Route note: "_el, route);
    el::io::printLine("Tokens:"_el);

    auto tokenStart = route.findFirstOf(tokenCharacters);
    while (!tokenStart.isNoIndex()) {
        auto tokenEnd = route.findFirstNotOf(tokenCharacters, tokenStart);
        if (tokenEnd.isNoIndex()) {
            tokenEnd = route.indexAt(el::StringSide::Back);
        }

        const auto token = route.slice(el::ByteRange{tokenStart, tokenEnd});
        el::io::printLine("  ["_el, tokenStart, ", "_el, tokenEnd, "): "_el, token);

        tokenStart = route.findFirstOf(tokenCharacters, tokenEnd);
    }
}

}
