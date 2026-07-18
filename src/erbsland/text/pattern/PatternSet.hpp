// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Limits.hpp"
#include "Range.hpp"

#include "impl/Require.hpp"

#include "../CharSet.hpp"

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
        for (const auto &range : ranges) {
            add(range.charRange());
        }
    }
    /// Create a set from an existing character set.
    explicit Set(const CharSet &charSet) {
        for (const auto &range : charSet.ranges()) {
            add(range);
        }
    }

public: // accessors
    /// The number of ranges in this set.
    [[nodiscard]] constexpr auto count() const noexcept -> std::size_t { return _count; }
    /// Access the range at the given index.
    [[nodiscard]] constexpr auto range(const std::size_t index) const noexcept -> CharRange { return _ranges[index]; }

private:
    constexpr void add(const CharRange range) {
        if (range.isEmpty()) {
            return;
        }
        impl::requirePattern(_count < cMaximumSetRanges, "StringEditor pattern set has too many ranges");
        _ranges[_count] = range;
        ++_count;
    }

private:
    std::array<CharRange, cMaximumSetRanges> _ranges{}; ///< The ranges in this set.
    std::size_t _count{};                               ///< The number of used ranges.
};

}
