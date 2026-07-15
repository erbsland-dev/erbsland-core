// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../CharRange.hpp"

namespace erbsland::text::pattern {

/// A character range element.
/// @tested{StringPatternTest}
class Range final {
public:
    /// Create an empty range.
    constexpr Range() noexcept = default;
    /// Create a range from two characters.
    constexpr Range(const Char first, const Char last) noexcept : _range{first, last} {}
    /// Create a range from two code points.
    constexpr Range(const char32_t first, const char32_t last) noexcept : Range{Char{first}, Char{last}} {}
    /// Create a range from an existing character range.
    explicit constexpr Range(const CharRange range) noexcept : _range{range} {}

public: // accessors
    /// Access the range.
    [[nodiscard]] constexpr auto charRange() const noexcept -> CharRange { return _range; }

private:
    CharRange _range; ///< The character range.
};

}
