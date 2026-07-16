// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Category.hpp"
#include "Character.hpp"

#include "../../../util/HashHelper.hpp"

#include <vector>

namespace erbsland::re::impl {

/// A character range
class CharRange {
public:
    /// Create a new character range.
    /// Automatically reorders `first` and `last` if required.
    /// @param first The first character of the range.
    /// @param last The last character of the range.
    constexpr CharRange(const text::Char first, const text::Char last) noexcept :
        _first{last < first ? last : first}, _last{last < first ? first : last} {}
    constexpr CharRange(const char32_t first, const char32_t last) noexcept :
        CharRange{text::Char{first}, text::Char{last}} {}
    constexpr explicit CharRange(const char32_t character) noexcept :
        CharRange{text::Char{character}, text::Char{character}} {}

    // defaults
    CharRange() = default;
    ~CharRange() = default;
    CharRange(const CharRange &) = default;
    auto operator=(const CharRange &) noexcept -> CharRange & = default;

public: // comparison operators
    constexpr auto operator==(const CharRange &other) const noexcept -> bool {
        return _first == other._first && _last == other._last;
    }
    constexpr auto operator!=(const CharRange &other) const noexcept -> bool {
        return _first != other._first || _last != other._last;
    }
    constexpr auto operator<(const CharRange &other) const noexcept -> bool {
        return _first == other._first ? _last < other._last : _first < other._first;
    }
    constexpr auto operator<=(const CharRange &other) const noexcept -> bool { return *this == other || *this < other; }
    constexpr auto operator>(const CharRange &other) const noexcept -> bool {
        return _first == other._first ? _last > other._last : _first > other._first;
    }
    constexpr auto operator>=(const CharRange &other) const noexcept -> bool { return *this == other || *this > other; }
    constexpr auto operator<=>(const CharRange &other) const noexcept -> std::strong_ordering {
        return _first <=> other._first != 0 ? _first <=> other._first : _last <=> other._last;
    }

public:
    /// If this is a single character.
    [[nodiscard]] constexpr auto isSingleChar() const noexcept -> bool { return _first == _last; }
    /// The first character in this range.
    [[nodiscard]] auto first() const noexcept -> text::Char { return _first; }
    /// The last character in this range.
    [[nodiscard]] auto last() const noexcept -> text::Char { return _last; }
    /// Test if the given character matches this range.
    [[nodiscard]] auto matches(text::Char character) const noexcept -> bool;
    /// Test if two sorted ranges overlap or are adjacent Unicode scalar ranges.
    [[nodiscard]] static constexpr auto canMerge(text::Char currentLast, text::Char nextFirst) noexcept -> bool {
        if (!currentLast.isValidUnicode() || !nextFirst.isValidUnicode()) {
            return false;
        }
        if (nextFirst <= currentLast) {
            return true;
        }
        const auto currentValue = currentLast.toRawValue();
        if (currentValue == 0x10FFFFU) {
            return false;
        }
        const auto nextScalarValue = currentValue == 0xD7FFU ? 0xE000U : currentValue + 1U;
        return nextFirst.toRawValue() == nextScalarValue;
    }
    /// Create a string representation of this range.
    /// If `first==last`, it outputs a single character, otherwise `<first>-<last>`
    /// For characters <= U+0020 and >= U+007F, it outputs a hex escape sequence `\uxxxx` or `\Uxxxxxxxx`
    /// A backslash is escaped as `\\`.
    /// The characters `^` and `-` are escaped as hex escape sequences.
    [[nodiscard]] auto toString() const -> text::String;

private:
    text::Char _first{0U}; ///< The first character of the range. Always the lower character.
    text::Char _last{0U};  ///< The last character of the range. Always the upper character.
};

}

namespace std {
template <>
struct hash<erbsland::re::impl::CharRange> {
    auto operator()(const erbsland::re::impl::CharRange &range) const noexcept -> std::size_t {
        return erbsland::util::createHash(range.first().toRawValue(), range.last().toRawValue());
    }
};
}
