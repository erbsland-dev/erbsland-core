// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Limits.hpp"
#include "Range.hpp"

#include "../CharSet.hpp"
#include "../StringLiteral.hpp"

#include "../../err/ParameterFailure.hpp"

#include <array>
#include <cstddef>
#include <initializer_list>

namespace erbsland::text::pattern {

/// A set element made of one or more ranges.
/// @tested{StringPatternTest}
class Set final {
public:
    /// Create an empty set.
    constexpr Set() noexcept = default;
    /// Create a set from a list of ranges.
    constexpr Set(std::initializer_list<Range> ranges) {
        if (ranges.size() > cMaximumSetRanges) {
            throw err::ParameterFailure{StringLiteral{"Set has too many ranges"}, StringLiteral{"ranges"}};
        }
        for (const auto &range : ranges) {
            _ranges[_count] = range.charRange();
            ++_count;
        }
    }
    /// Create a set from an existing character set.
    explicit Set(const CharSet &charSet) {
        charSet.forEach([this](const CharRange range) -> void {
            if (_count >= cMaximumSetRanges) {
                throw err::ParameterFailure{StringLiteral{"Set has too many ranges"}, StringLiteral{"charSet"}};
            }
            _ranges[_count] = range;
            ++_count;
        });
    }

public: // accessors
    /// The number of ranges in this set.
    [[nodiscard]] constexpr auto count() const noexcept -> std::size_t { return _count; }
    /// Access the range at the given index.
    [[nodiscard]] constexpr auto range(const std::size_t index) const noexcept -> CharRange { return _ranges[index]; }

private:
    std::array<CharRange, cMaximumSetRanges> _ranges{}; ///< The ranges in this set.
    std::size_t _count{};                               ///< The number of used ranges.
};

}
