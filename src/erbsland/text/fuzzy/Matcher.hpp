// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Match.hpp"
#include "Matcher_fwd.hpp"

#include "../CharCompareFn.hpp"
#include "../String.hpp"
#include "../StringList.hpp"
#include "../u32/U32String.hpp"

#include "../../unit/CpLength.hpp"
#include "../../unit/ItemCount.hpp"

#include <cstddef>
#include <vector>

namespace erbsland::text::fuzzy {

/// Find text candidates using bounded Damerau-Levenshtein distance.
///
/// Matching operates on decoded Unicode code points. Adjacent transpositions count as one edit. Results are ordered by
/// distance and retain candidate order for ties.
/// @tested{FuzzyMatcherTest}
class Matcher final {
public:
    /// Create a matcher for an empty pattern.
    Matcher() = default;
    /// Create a matcher for a pattern.
    /// @param pattern The text to compare with candidates.
    explicit Matcher(text::String pattern) noexcept : _pattern{std::move(pattern)} {}

    // defaults
    ~Matcher() = default;
    Matcher(const Matcher &) = default;
    Matcher(Matcher &&) noexcept = default;
    auto operator=(const Matcher &) -> Matcher & = default;
    auto operator=(Matcher &&) noexcept -> Matcher & = default;

public: // accessors
    /// Get the pattern.
    [[nodiscard]] auto pattern() const noexcept -> const text::String & { return _pattern; }
    /// Set the pattern.
    auto setPattern(text::String pattern) noexcept -> Matcher &;
    /// Get the maximum accepted edit distance.
    [[nodiscard]] auto maximumDistance() const noexcept -> unit::CpLength { return _maximumDistance; }
    /// Set the maximum accepted edit distance.
    auto setMaximumDistance(unit::CpLength maximumDistance) noexcept -> Matcher &;
    /// Get the maximum number of returned matches.
    [[nodiscard]] auto maximumResults() const noexcept -> unit::ItemCount { return _maximumResults; }
    /// Set the maximum number of returned matches.
    auto setMaximumResults(unit::ItemCount maximumResults) noexcept -> Matcher &;
    /// Get the optional decoded-character comparison function.
    [[nodiscard]] auto comparisonFn() const noexcept -> CharCompareFn { return _comparisonFn; }
    /// Set the decoded-character comparison function. Empty selects exact comparison.
    auto setComparisonFn(CharCompareFn comparisonFn) noexcept -> Matcher &;

public:
    /// Find matching candidates.
    /// @param candidates Candidate text in preferred tie order.
    /// @return Accepted, deduplicated matches ordered by distance and candidate order.
    [[nodiscard]] auto findMatches(const text::StringList &candidates) const -> MatchList;

private:
    /// Internal match paired with its original candidate order.
    struct RankedMatch final {
        Match match;
        std::size_t order{0};
    };

    /// Compute a bounded edit distance.
    [[nodiscard]] auto distance(const text::U32String &candidate) const -> unit::CpLength;
    /// Compare two decoded characters using the configured comparison.
    [[nodiscard]] auto charactersEqual(text::Char left, text::Char right) const noexcept -> bool;
    /// Test whether equivalent candidate text was already collected.
    [[nodiscard]] auto containsEquivalent(const std::vector<RankedMatch> &matches, const text::String &text) const
        -> bool;

private:
    text::String _pattern;                                        ///< The reference text.
    unit::CpLength _maximumDistance{unit::CpLength::infinite()};  ///< Maximum accepted edit distance.
    unit::ItemCount _maximumResults{unit::ItemCount::infinite()}; ///< Maximum returned matches.
    CharCompareFn _comparisonFn{nullptr};                         ///< Optional character comparison.
};

}
