// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/text/Char.hpp>
#include <erbsland/text/fuzzy/Matcher.hpp>
#include <erbsland/text/StringList.hpp>

namespace demo {

/// Suggest nearby names from a small, known list of sound terms.
///
/// `Matcher` compares a pattern with each candidate by Unicode code point. Insertions, deletions, substitutions,
/// and adjacent transpositions each count as one edit. A distance limit keeps unrelated names out of suggestions.
void suggestSoundNames() {
    const auto candidates = el::StringList{"kaiku"_el, "sointu"_el, "sävel"_el, "kaiut"_el};
    const auto matches = el::fuzzy::Matcher{"kaik"_el}.setMaximumDistance(el::CpLength{1U}).findMatches(candidates);
    for (const auto &match : matches) {
        el::io::printLine(match.candidate(), " (edits: "_el, match.distance().toSizeT(), ")"_el);
    }
}

/// Tune the distance threshold, result count, comparison function, and pattern of a matcher.
///
/// The default comparison is exact and case-sensitive. An ASCII-folding callback makes ASCII case differences
/// equivalent and also controls which candidate spellings count as duplicates. The first equivalent spelling wins.
void tuneSuggestions() {
    auto matcher = el::fuzzy::Matcher{};
    matcher.setPattern("KAIKU"_el)
        .setMaximumDistance(el::CpLength{1U})
        .setMaximumResults(el::ItemCount{2U})
        .setComparisonFn(el::Char::compareAsciiFolded);

    const auto candidates = el::StringList{"kaiku"_el, "KAIKU"_el, "kaiuk"_el, "kaikuu"_el, "sointu"_el};
    const auto matches = matcher.findMatches(candidates);
    for (const auto &match : matches) {
        el::io::printLine(match.text(), " (edits: "_el, match.distance().toSizeT(), ")"_el);
    }
}

/// Read ranked `Match` values without losing the original candidate spelling.
///
/// A distance of zero means an exact match under the selected character comparison; one means one edit. Equal
/// distances keep input order. Each result holds the original candidate text, so it can be shown directly to a user.
void inspectMatches() {
    const auto candidates = el::StringList{u8"sävel"_el, u8"savel"_el, u8"säevl"_el, u8"säveli"_el};
    const auto matches = el::fuzzy::Matcher{u8"sävel"_el}.setMaximumDistance(el::CpLength{1U}).findMatches(candidates);
    for (const auto &match : matches) {
        el::io::printLine("Candidate: "_el, match.text(), ", distance: "_el, match.distance().toSizeT());
    }
}

}
