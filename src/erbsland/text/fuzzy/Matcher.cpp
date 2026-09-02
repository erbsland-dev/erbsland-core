// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Matcher.hpp"

#include "../StringConverter.hpp"

#include <algorithm>
#include <compare>
#include <utility>
#include <vector>

namespace erbsland::text::fuzzy {

auto Matcher::setPattern(text::String pattern) noexcept -> Matcher & {
    _pattern = std::move(pattern);
    return *this;
}

auto Matcher::setMaximumDistance(const unit::CpLength maximumDistance) noexcept -> Matcher & {
    _maximumDistance = maximumDistance;
    return *this;
}

auto Matcher::setMaximumResults(const unit::ItemCount maximumResults) noexcept -> Matcher & {
    _maximumResults = maximumResults;
    return *this;
}

auto Matcher::setComparisonFn(const CharCompareFn comparisonFn) noexcept -> Matcher & {
    _comparisonFn = comparisonFn;
    return *this;
}

auto Matcher::findMatches(const text::StringList &candidates) const -> MatchList {
    auto rankedMatches = std::vector<RankedMatch>{};
    rankedMatches.reserve(candidates.count().toSizeT());
    auto order = std::size_t{0};
    for (const auto &candidate : candidates) {
        if (containsEquivalent(rankedMatches, candidate)) {
            ++order;
            continue;
        }
        const auto matchDistance = distance(text::StringConverter{candidate}.toU32String());
        if (matchDistance <= _maximumDistance) {
            rankedMatches.emplace_back(RankedMatch{Match{candidate, matchDistance}, order});
        }
        ++order;
    }
    std::ranges::stable_sort(rankedMatches, [](const RankedMatch &left, const RankedMatch &right) -> bool {
        if (left.match.distance() != right.match.distance()) {
            return left.match.distance() < right.match.distance();
        }
        return left.order < right.order;
    });

    auto result = MatchList{};
    const auto resultLimit =
        _maximumResults.isInfinite() ? rankedMatches.size() : std::min(rankedMatches.size(), _maximumResults.toSizeT());
    result.reserve(unit::ItemCount::fromSizeT(resultLimit));
    for (auto index = std::size_t{0}; index < resultLimit; ++index) {
        result.append(rankedMatches[index].match);
    }
    return result;
}

auto Matcher::distance(const text::U32String &candidate) const -> unit::CpLength {
    const auto pattern = text::StringConverter{_pattern}.toU32String();
    const auto patternLength = pattern.characterLength().toSizeT();
    const auto candidateLength = candidate.characterLength().toSizeT();
    if (patternLength == 0U) {
        return unit::CpLength::fromSizeT(candidateLength);
    }
    if (candidateLength == 0U) {
        return unit::CpLength::fromSizeT(patternLength);
    }

    auto previousPrevious = std::vector<std::size_t>(candidateLength + 1U);
    auto previous = std::vector<std::size_t>(candidateLength + 1U);
    auto current = std::vector<std::size_t>(candidateLength + 1U);
    for (auto column = std::size_t{0}; column <= candidateLength; ++column) {
        previous[column] = column;
    }
    for (auto row = std::size_t{1U}; row <= patternLength; ++row) {
        current[0] = row;
        const auto patternCharacter = pattern.charAt(unit::CpIndex::fromSizeT(row - 1U));
        for (auto column = std::size_t{1U}; column <= candidateLength; ++column) {
            const auto candidateCharacter = candidate.charAt(unit::CpIndex::fromSizeT(column - 1U));
            const auto substitutionCost = charactersEqual(patternCharacter, candidateCharacter) ? 0U : 1U;
            current[column] = std::min({
                previous[column] + 1U,
                current[column - 1U] + 1U,
                previous[column - 1U] + substitutionCost,
            });
            if (row > 1U && column > 1U &&
                charactersEqual(patternCharacter, candidate.charAt(unit::CpIndex::fromSizeT(column - 2U))) &&
                charactersEqual(pattern.charAt(unit::CpIndex::fromSizeT(row - 2U)), candidateCharacter)) {
                current[column] = std::min(current[column], previousPrevious[column - 2U] + 1U);
            }
        }
        previousPrevious.swap(previous);
        previous.swap(current);
    }
    return unit::CpLength::fromSizeT(previous[candidateLength]);
}

auto Matcher::charactersEqual(const text::Char left, const text::Char right) const noexcept -> bool {
    if (_comparisonFn == nullptr) {
        return left == right;
    }
    return _comparisonFn(left, right) == std::strong_ordering::equal;
}

auto Matcher::containsEquivalent(const std::vector<RankedMatch> &matches, const text::String &candidate) const -> bool {
    return std::ranges::any_of(matches, [this, &candidate](const RankedMatch &entry) -> bool {
        return entry.match.text().compare(candidate, _comparisonFn) == std::strong_ordering::equal;
    });
}

}
