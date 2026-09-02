// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Char.hpp"

#include <algorithm>
#include <compare>
#include <tuple>

namespace erbsland::text {

/// A range of Unicode scalar values.
/// Empty ranges are represented by invalid endpoint characters. Invalid input creates an empty range, while valid
/// endpoints are ordered automatically.
/// @seedoc{/reference/text/characters}
/// @tested{CharRangeTest}
class CharRange final {
public:
    /// Create an empty range.
    constexpr CharRange() noexcept = default;
    /// Create a range containing one character.
    explicit constexpr CharRange(const Char character) noexcept : _from{character}, _to{character} {
        if (!character.isValidUnicode()) {
            clear();
        }
    }
    /// Create a range from two characters, ordering the endpoints automatically.
    constexpr CharRange(const Char first, const Char second) noexcept : _from{first}, _to{second} {
        if (!first.isValidUnicode() || !second.isValidUnicode()) {
            clear();
            return;
        }
        if (_to < _from) {
            const auto temporary = _from;
            _from = _to;
            _to = temporary;
        }
    }

    // defaults
    ~CharRange() = default;
    CharRange(const CharRange &) = default;
    CharRange(CharRange &&) = default;
    auto operator=(const CharRange &) -> CharRange & = default;
    auto operator=(CharRange &&) -> CharRange & = default;

public: // operators
    constexpr auto operator<=>(const CharRange &other) const noexcept -> std::strong_ordering = default;
    constexpr auto operator==(const CharRange &other) const noexcept -> bool = default;

public: // tests
    /// Test if this range is empty.
    [[nodiscard]] constexpr auto isEmpty() const noexcept -> bool {
        return !_from.isValidUnicode() || !_to.isValidUnicode() || _to < _from;
    }
    /// Test if this range contains exactly one character.
    [[nodiscard]] constexpr auto isSingleChar() const noexcept -> bool { return !isEmpty() && _from == _to; }
    /// Test if the character is contained in this range.
    [[nodiscard]] constexpr auto contains(const Char character) const noexcept -> bool {
        return !isEmpty() && character.isValidUnicode() && character >= _from && character <= _to;
    }
    /// Test if this range overlaps another range.
    [[nodiscard]] constexpr auto overlaps(const CharRange &other) const noexcept -> bool {
        return !isEmpty() && !other.isEmpty() && !(other._to < _from || other._from > _to);
    }
    /// Test if this range is directly adjacent to another range in Unicode scalar order.
    [[nodiscard]] constexpr auto isAdjacentTo(const CharRange &other) const noexcept -> bool {
        return !isEmpty() && !other.isEmpty() &&
            (nextScalarValue(_to.toRawValue()) == other._from.toRawValue() ||
                nextScalarValue(other._to.toRawValue()) == _from.toRawValue());
    }
    /// Test if this range can be merged with another range.
    [[nodiscard]] constexpr auto canMergeWith(const CharRange &other) const noexcept -> bool {
        return overlaps(other) || isAdjacentTo(other);
    }
    /// Test if this range contains characters affected by Unicode simple case folding.
    /// @usesunidb{Uses generated Unicode Character Database character metadata.}
    [[nodiscard]] auto containsCaseFoldableCharacters() const noexcept -> bool;
    /// Test if this range contains characters affected by Unicode simple lowercase mapping.
    /// @usesunidb{Uses generated Unicode Character Database character metadata.}
    [[nodiscard]] auto containsLowercaseMappableCharacters() const noexcept -> bool;
    /// Test if this range contains characters affected by Unicode simple uppercase mapping.
    /// @usesunidb{Uses generated Unicode Character Database character metadata.}
    [[nodiscard]] auto containsUppercaseMappableCharacters() const noexcept -> bool;

public: // accessors
    /// Access the first character in the range.
    [[nodiscard]] constexpr auto from() const noexcept -> Char { return _from; }
    /// Access the last character in the range.
    [[nodiscard]] constexpr auto to() const noexcept -> Char { return _to; }
    /// Access both range endpoints.
    [[nodiscard]] constexpr auto values() const noexcept -> std::tuple<Char, Char> { return {_from, _to}; }
    /// Merge this range with another range, returning an empty range if they cannot be merged.
    [[nodiscard]] constexpr auto mergedWith(const CharRange &other) const noexcept -> CharRange {
        if (!canMergeWith(other)) {
            return {};
        }
        return {std::min(_from, other._from), std::max(_to, other._to)};
    }

public: // factories
    /// Create a range that covers all Unicode scalar values.
    [[nodiscard]] constexpr static auto all() noexcept -> CharRange { return {Char{0U}, Char{0x10FFFFU}}; }

private:
    static constexpr auto cEmptyValue = char32_t{0x110000U};

    /// Clear this range.
    constexpr void clear() noexcept {
        _from = Char{cEmptyValue};
        _to = Char{cEmptyValue};
    }
    /// Get the next valid Unicode scalar value.
    [[nodiscard]] constexpr static auto nextScalarValue(const char32_t value) noexcept -> char32_t {
        if (value == 0xD7FFU) {
            return 0xE000U;
        }
        if (value < 0x10FFFFU) {
            return value + 1U;
        }
        return cEmptyValue;
    }

private:
    Char _from{cEmptyValue}; ///< The first character in the range.
    Char _to{cEmptyValue};   ///< The last character in the range.
};

}
