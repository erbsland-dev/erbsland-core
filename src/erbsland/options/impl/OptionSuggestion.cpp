// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionSuggestion.hpp"

#include "../../text/CaseSensitivity.hpp"
#include "../../text/fuzzy/Matcher.hpp"

namespace erbsland::options::impl {

auto findOptionSuggestions(const text::String &pattern, const text::StringList &candidates) -> text::StringList {
    const auto length = pattern.characterLength().toSizeT();
    const auto maximumDistance = length <= 4U ? 1U : (length <= 8U ? 2U : 3U);
    const auto matches = text::fuzzy::Matcher{pattern}
                             .setMaximumDistance(unit::CpLength::fromSizeT(maximumDistance))
                             .setMaximumResults(unit::ItemCount{3U})
                             .setComparisonFn(text::cCaseInsensitive.comparisonFn())
                             .findMatches(candidates);
    auto result = text::StringList{};
    result.reserve(matches.count());
    for (const auto &match : matches) {
        result.append(match.text());
    }
    return result;
}

}
