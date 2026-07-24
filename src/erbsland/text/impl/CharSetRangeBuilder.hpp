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

/// Builds a character set from ranges supplied in ascending order.
/// @tested{CharSetTest}
class CharSetRangeBuilder final {
public:
    /// Create a builder with an upper bound for the final number of ranges.
    /// @param maximumRangeCount The maximum number of normalized ranges that can be emitted.
    explicit CharSetRangeBuilder(std::size_t maximumRangeCount) noexcept;

    // defaults/deletions
    ~CharSetRangeBuilder();
    CharSetRangeBuilder(const CharSetRangeBuilder &) = delete;
    CharSetRangeBuilder(CharSetRangeBuilder &&) = delete;
    auto operator=(const CharSetRangeBuilder &) -> CharSetRangeBuilder & = delete;
    auto operator=(CharSetRangeBuilder &&) -> CharSetRangeBuilder & = delete;

public:
    /// Add the next range in ascending order and merge it with the previous range when possible.
    /// @param range The next normalized or unnormalized range.
    void add(CharRange range);
    /// Finish construction and return the normalized character set.
    /// @return The completed character set.
    [[nodiscard]] auto take() -> CharSet;

private:
    [[nodiscard]] auto count() const noexcept -> std::size_t;
    [[nodiscard]] auto last() noexcept -> CharRange &;
    void append(CharRange range);
    void promote();

private:
    std::size_t _maximumRangeCount{};      ///< The maximum number of output ranges.
    CharSet::InlineRanges _inlineRanges{}; ///< Inline output before promotion.
    CharSet::RangeDataPtr _sharedRanges{}; ///< Shared output after promotion.
};

}
