// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../CharSet.hpp"

#include <cstddef>

namespace erbsland::text::impl {

/// Counts normalized ranges supplied in ascending order.
/// @tested{CharSetTest}
class CharSetRangeCounter final {
public:
    /// Add the next range in ascending order.
    /// @param range The next normalized or unnormalized range.
    void add(CharRange range) noexcept;
    /// Return the number of normalized ranges seen so far.
    /// @return The normalized range count.
    [[nodiscard]] auto count() const noexcept -> std::size_t { return _count; }

private:
    CharRange _lastRange{}; ///< The last normalized range.
    std::size_t _count{};   ///< The number of normalized ranges.
};

}
