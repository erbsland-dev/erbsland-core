// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16StringDataView.hpp"

#include "../../../unit/ItemCount.hpp"
#include "../../../unit/U16Index.hpp"
#include "../../../unit/U16Range.hpp"
#include "../../AsciiCategory.hpp"
#include "../../CharCompareFn.hpp"
#include "../../CharSet.hpp"
#include "../../StringSide.hpp"

#include <compare>
#include <span>

namespace erbsland::text::impl {

/// Comparison and containment algorithms on UTF-16 string data.
/// @tested{U16StringTest StringSeparatorSplitTest}
class U16StringComparisonTools final {
public:
    /// A set of Unicode code-points for decoded character lookups.
    using CharacterSet = CharSet;

public:
    /// Create comparison tools for `data`.
    explicit constexpr U16StringComparisonTools(const U16StringDataView &data) noexcept : _data{data} {}

public: // comparison
    /// Compare two strings by decoded code point, replacing malformed UTF-16 with `Char::replacement()`.
    [[nodiscard]] auto compare(const U16StringDataView &other, CharCompareFn compareFn = {}) const noexcept
        -> std::strong_ordering;

public: // find
    /// Find the first decoded character sequence.
    [[nodiscard]] auto find(const U16StringDataView &text, CharCompareFn compareFn = {}) const noexcept
        -> unit::U16Index;
    /// Find the first decoded character sequence at or after the given UTF-16 data index.
    [[nodiscard]] auto find(
        const U16StringDataView &text, unit::U16Index start, CharCompareFn compareFn = {}) const noexcept
        -> unit::U16Index;

    /// Find the first non-empty separator and retain its native boundaries.
    /// @param text The separator data.
    /// @param compareFn Optional decoded character comparison function.
    /// @return The relative match range, or no-range for an absent or empty separator.
    [[nodiscard]] auto findFirstRange(const U16StringDataView &text, CharCompareFn compareFn = {}) const noexcept
        -> unit::U16Range;
    /// Find the rightmost non-empty separator, including overlapping matches.
    /// @param text The separator data.
    /// @param compareFn Optional decoded character comparison function.
    /// @return The relative match range, or no-range for an absent or empty separator.
    [[nodiscard]] auto findLastRange(const U16StringDataView &text, CharCompareFn compareFn = {}) const noexcept
        -> unit::U16Range;
    /// Match a decoded separator at one candidate position.
    /// @param data The source data span.
    /// @param start The candidate's native start index.
    /// @param text The separator span.
    /// @param compareFn Optional decoded character comparison function.
    /// @return The actual source end of the match, or no-index on mismatch.
    [[nodiscard]] static auto endOfTextMatch(
        std::span<const char16_t> data,
        unit::U16Index start,
        std::span<const char16_t> text,
        CharCompareFn compareFn) noexcept -> unit::U16Index;

public: // tests
    /// Test if the view starts with the given decoded text.
    [[nodiscard]] auto startsWith(const U16StringDataView &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// Test if the view starts with the given decoded character.
    [[nodiscard]] auto startsWith(Char character) const noexcept -> bool;
    /// Test if the view ends with the given decoded text.
    [[nodiscard]] auto endsWith(const U16StringDataView &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// Test if the view ends with the given decoded character.
    [[nodiscard]] auto endsWith(Char character) const noexcept -> bool;
    /// Test if the view contains the given decoded text.
    [[nodiscard]] auto contains(const U16StringDataView &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// Test if the view contains the given decoded character.
    [[nodiscard]] auto contains(Char character) const noexcept -> bool;
    /// Count non-overlapping occurrences of the given decoded text.
    [[nodiscard]] auto count(const U16StringDataView &other, CharCompareFn compareFn = {}) const noexcept
        -> unit::ItemCount;
    /// Count occurrences of the given decoded character.
    [[nodiscard]] auto count(Char character) const noexcept -> unit::ItemCount;
    /// Test if the view contains one decoded character from the given character set.
    [[nodiscard]] auto containsOneOf(const CharSet &characters) const noexcept -> bool;
    /// Test if the view contains only decoded characters from the given character set.
    [[nodiscard]] auto containsOnly(const CharSet &characters) const noexcept -> bool;
    /// Test if the view contains only decoded characters from an ASCII category.
    [[nodiscard]] auto containsOnly(AsciiCategory category) const noexcept -> bool;

public: // character traversal
    /// Test if UTF-16 data contains one decoded character from the given character set.
    [[nodiscard]] static auto containsOneDecodedCharacter(
        std::span<const char16_t> data, const CharacterSet &characters) -> bool;

private:
    /// Search in the selected direction and retain the match boundaries.
    [[nodiscard]] auto findTextRange(
        const U16StringDataView &text, StringSide side, CharCompareFn compareFn) const noexcept -> unit::U16Range;

private:
    /// Compare two decoded UTF-16 spans with an optional character comparator.
    [[nodiscard]] static auto compareDecodedSpans(
        std::span<const char16_t> left, std::span<const char16_t> right, CharCompareFn compareFn) noexcept
        -> std::strong_ordering;
    /// Test if the haystack at the candidate start matches the decoded needle.
    [[nodiscard]] static auto matchesDecodedSpan(
        std::span<const char16_t> haystack,
        unit::U16Index candidateStart,
        std::span<const char16_t> needle,
        CharCompareFn compareFn) noexcept -> bool;
    /// Test if the haystack starts with the decoded needle.
    [[nodiscard]] static auto startsWithDecodedSpan(
        std::span<const char16_t> haystack, std::span<const char16_t> needle, CharCompareFn compareFn) noexcept -> bool;
    /// Test if the haystack ends with the decoded needle.
    [[nodiscard]] static auto endsWithDecodedSpan(
        std::span<const char16_t> haystack, std::span<const char16_t> needle, CharCompareFn compareFn) noexcept -> bool;
    /// Return the haystack position after a matching decoded needle.
    [[nodiscard]] static auto endOfMatch(
        std::span<const char16_t> haystack, unit::U16Index start, std::span<const char16_t> needle) noexcept
        -> unit::U16Index;
    /// Test if two decoded characters are equal according to the optional comparison function.
    [[nodiscard]] static auto charactersEqual(Char left, Char right, CharCompareFn compareFn) noexcept -> bool;

private:
    U16StringDataView _data;
};

}
