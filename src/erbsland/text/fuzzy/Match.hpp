// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Match_fwd.hpp"

#include "../String.hpp"

#include "../../unit/CpLength.hpp"
#include "../../util/List.hpp"

#include <utility>

namespace erbsland::text::fuzzy {

/// One fuzzy text match and its edit distance.
/// @tested{FuzzyMatcherTest}
class Match final {
public:
    /// Create an empty match.
    Match() = default;
    /// Create a match.
    /// @param text The matching candidate text.
    /// @param distance The Damerau-Levenshtein edit distance.
    Match(text::String text, unit::CpLength distance) noexcept : _text{std::move(text)}, _distance{distance} {}

    // defaults
    ~Match() = default;
    Match(const Match &) = default;
    Match(Match &&) noexcept = default;
    auto operator=(const Match &) -> Match & = default;
    auto operator=(Match &&) noexcept -> Match & = default;

public: // accessors
    /// Get the matching candidate text.
    [[nodiscard]] auto text() const noexcept -> const text::String & { return _text; }
    /// Get the matching candidate text.
    [[nodiscard]] auto candidate() const noexcept -> const text::String & { return _text; }
    /// Get the edit distance from the pattern.
    [[nodiscard]] auto distance() const noexcept -> unit::CpLength { return _distance; }

private:
    text::String _text;                                   ///< The matching candidate.
    unit::CpLength _distance{unit::CpLength::infinite()}; ///< The edit distance.
};

}
