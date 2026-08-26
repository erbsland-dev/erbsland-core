// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8StringDataView.hpp"

#include "../../../unit/ByteIndex.hpp"
#include "../../../unit/ItemCount.hpp"
#include "../../AsciiCategory.hpp"
#include "../../CharCompareFn.hpp"
#include "../../CharSet.hpp"

#include <compare>
#include <span>

namespace erbsland::text::impl {

/// Comparison and containment algorithms on UTF-8 string data.
/// @tested{U8StringTest}
class U8StringComparisonTools final {
public:
    /// A set of Unicode code-points for decoded character lookups.
    using CharacterSet = CharSet;

public:
    /// Create comparison tools for `data`.
    explicit constexpr U8StringComparisonTools(const U8StringDataView &data) noexcept : _data{data} {}

public: // comparison
    /// Compare two strings by decoded code point, replacing malformed UTF-8 with `Char::replacement()`.
    [[nodiscard]] auto compare(const U8StringDataView &other, CharCompareFn compareFn = {}) const noexcept
        -> std::strong_ordering;

public: // find
    /// Find the first decoded character sequence.
    [[nodiscard]] auto find(const U8StringDataView &text, CharCompareFn compareFn = {}) const noexcept
        -> unit::ByteIndex;
    /// Find the first decoded character sequence at or after the given byte index.
    [[nodiscard]] auto find(
        const U8StringDataView &text, unit::ByteIndex start, CharCompareFn compareFn = {}) const noexcept
        -> unit::ByteIndex;

public: // tests
    /// Test if the view starts with the given decoded text.
    [[nodiscard]] auto startsWith(const U8StringDataView &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// Test if the view starts with the given decoded character.
    [[nodiscard]] auto startsWith(Char character) const noexcept -> bool;
    /// Test if the view ends with the given decoded text.
    [[nodiscard]] auto endsWith(const U8StringDataView &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// Test if the view ends with the given decoded character.
    [[nodiscard]] auto endsWith(Char character) const noexcept -> bool;
    /// Test if the view contains the given decoded text.
    [[nodiscard]] auto contains(const U8StringDataView &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// Test if the view contains the given decoded character.
    [[nodiscard]] auto contains(Char character) const noexcept -> bool;
    /// Count non-overlapping occurrences of the given decoded text.
    [[nodiscard]] auto count(const U8StringDataView &other, CharCompareFn compareFn = {}) const noexcept
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
    /// Test if UTF-8 data contains one decoded character from the given character set.
    [[nodiscard]] static auto containsOneDecodedCharacter(std::span<const char> data, const CharacterSet &characters)
        -> bool;

private:
    /// Compare two decoded spans with tolerant UTF-8 decoding.
    [[nodiscard]] static auto compareDecodedSpans(
        std::span<const char> left, std::span<const char> right, CharCompareFn compareFn) noexcept
        -> std::strong_ordering;
    /// Test if the haystack at the candidate start matches the decoded needle.
    [[nodiscard]] static auto matchesDecodedSpan(
        std::span<const char> haystack,
        unit::ByteIndex candidateStart,
        std::span<const char> needle,
        CharCompareFn compareFn) noexcept -> bool;
    /// Test if the haystack starts with the decoded needle.
    [[nodiscard]] static auto startsWithDecodedSpan(
        std::span<const char> haystack, std::span<const char> needle, CharCompareFn compareFn) noexcept -> bool;
    /// Test if the haystack ends with the decoded needle.
    [[nodiscard]] static auto endsWithDecodedSpan(
        std::span<const char> haystack, std::span<const char> needle, CharCompareFn compareFn) noexcept -> bool;
    /// Return the haystack position after a matching decoded needle.
    [[nodiscard]] static auto endOfMatch(
        std::span<const char> haystack, unit::ByteIndex start, std::span<const char> needle) noexcept
        -> unit::ByteIndex;
    /// Test if two decoded characters are equal according to the optional comparison function.
    [[nodiscard]] static auto charactersEqual(Char left, Char right, CharCompareFn compareFn) noexcept -> bool;

private:
    U8StringDataView _data;
};

}
