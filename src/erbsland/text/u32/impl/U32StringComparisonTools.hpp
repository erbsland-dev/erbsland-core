// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U32StringDataView.hpp"

#include "../../../unit/CpIndex.hpp"
#include "../../../unit/ItemCount.hpp"
#include "../../AsciiCategory.hpp"
#include "../../CharCompareFn.hpp"
#include "../../CharSet.hpp"

#include <compare>
#include <span>

namespace erbsland::text::impl {

/// Comparison and containment algorithms on UTF-32 string data.
/// @tested{U32StringTest}
class U32StringComparisonTools final {
public:
    /// A set of Unicode code-points for decoded character lookups.
    using CharacterSet = CharSet;

public:
    /// Create comparison tools for `data`.
    explicit constexpr U32StringComparisonTools(const U32StringDataView &data) noexcept : _data{data} {}

public: // comparison
    /// Compare two strings by decoded code point, replacing malformed UTF-32 with `Char::replacement()`.
    [[nodiscard]] auto compare(const U32StringDataView &other, CharCompareFn compareFn = {}) const noexcept
        -> std::strong_ordering;

public: // find
    /// Find the first decoded character sequence.
    [[nodiscard]] auto find(const U32StringDataView &text, CharCompareFn compareFn = {}) const noexcept
        -> unit::CpIndex;
    /// Find the first decoded character sequence at or after the given UTF-32 data index.
    [[nodiscard]] auto find(
        const U32StringDataView &text, unit::CpIndex start, CharCompareFn compareFn = {}) const noexcept
        -> unit::CpIndex;

public: // tests
    /// Test if the view starts with the given decoded text.
    [[nodiscard]] auto startsWith(const U32StringDataView &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// Test if the view starts with the given decoded character.
    [[nodiscard]] auto startsWith(Char character) const noexcept -> bool;
    /// Test if the view ends with the given decoded text.
    [[nodiscard]] auto endsWith(const U32StringDataView &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// Test if the view ends with the given decoded character.
    [[nodiscard]] auto endsWith(Char character) const noexcept -> bool;
    /// Test if the view contains the given decoded text.
    [[nodiscard]] auto contains(const U32StringDataView &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// Test if the view contains the given decoded character.
    [[nodiscard]] auto contains(Char character) const noexcept -> bool;
    /// Count non-overlapping occurrences of the given decoded text.
    [[nodiscard]] auto count(const U32StringDataView &other, CharCompareFn compareFn = {}) const noexcept
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
    /// Test if UTF-32 data contains one decoded character from the given character set.
    [[nodiscard]] static auto containsOneDecodedCharacter(
        std::span<const char32_t> data, const CharacterSet &characters) -> bool;

private:
    /// Compare two decoded UTF-32 spans with an optional character comparator.
    [[nodiscard]] static auto compareDecodedSpans(
        std::span<const char32_t> left, std::span<const char32_t> right, CharCompareFn compareFn) noexcept
        -> std::strong_ordering;
    /// Test if the haystack at the candidate start matches the decoded needle.
    [[nodiscard]] static auto matchesDecodedSpan(
        std::span<const char32_t> haystack,
        unit::CpIndex candidateStart,
        std::span<const char32_t> needle,
        CharCompareFn compareFn) noexcept -> bool;
    /// Test if the haystack starts with the decoded needle.
    [[nodiscard]] static auto startsWithDecodedSpan(
        std::span<const char32_t> haystack, std::span<const char32_t> needle, CharCompareFn compareFn) noexcept -> bool;
    /// Test if the haystack ends with the decoded needle.
    [[nodiscard]] static auto endsWithDecodedSpan(
        std::span<const char32_t> haystack, std::span<const char32_t> needle, CharCompareFn compareFn) noexcept -> bool;
    /// Return the haystack position after a matching decoded needle.
    [[nodiscard]] static auto endOfMatch(
        std::span<const char32_t> haystack, unit::CpIndex start, std::span<const char32_t> needle) noexcept
        -> unit::CpIndex;
    /// Test if two decoded characters are equal according to the optional comparison function.
    [[nodiscard]] static auto charactersEqual(Char left, Char right, CharCompareFn compareFn) noexcept -> bool;

private:
    U32StringDataView _data;
};

}
