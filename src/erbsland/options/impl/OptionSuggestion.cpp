// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionSuggestion.hpp"

#include "../../text/CaseSensitivity.hpp"
#include "../../text/fuzzy/Matcher.hpp"

namespace erbsland::options::impl {

using namespace text;

auto findOptionSuggestions(const String &pattern, const StringList &candidates) -> StringList {
    const auto length = pattern.characterLength().toSizeT();
    const auto maximumDistance = length <= 4U ? 1U : (length <= 8U ? 2U : 3U);
    const auto matches = fuzzy::Matcher{pattern}
                             .setMaximumDistance(unit::CpCount::fromSizeT(maximumDistance))
                             .setMaximumResults(unit::ItemCount{3U})
                             .setComparisonFn(cCaseInsensitive.comparisonFn())
                             .findMatches(candidates);
    auto result = StringList{};
    result.reserve(matches.count());
    for (const auto &match : matches) {
        result.append(match.text());
    }
    return result;
}

}
