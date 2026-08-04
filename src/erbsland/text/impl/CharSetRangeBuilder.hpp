// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CharSetRangeBuilder_fwd.hpp"

#include "../CharSet.hpp"

#include <cstddef>

namespace erbsland::text::impl {

/// Builds a character set from ranges supplied in ascending order.
/// @tested{CharSetTest}
class CharSetRangeBuilder final {
public:
    /// Create a builder with an upper bound for the final number of ranges.
    /// @param maximumRangeCount The maximum number of normalized ranges that can be emitted.
    explicit CharSetRangeBuilder(std::size_t maximumRangeCount) noexcept;
    /// default dtor
    ~CharSetRangeBuilder();

    // defaults/deletions
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
    /// Return the current number of collected ranges.
    [[nodiscard]] auto count() const noexcept -> std::size_t;
    /// Access the final collected range.
    [[nodiscard]] auto last() noexcept -> CharRange &;
    /// Append one unmerged range.
    void append(CharRange range);
    /// Promote inline ranges to shared range storage.
    void promote();

private:
    std::size_t _maximumRangeCount{};      ///< The maximum number of output ranges.
    CharSet::InlineRanges _inlineRanges{}; ///< Inline output before promotion.
    CharSet::RangeDataPtr _sharedRanges{}; ///< Shared output after promotion.
};

}
