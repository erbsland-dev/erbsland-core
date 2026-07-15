// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LayoutSemantics.hpp"

#include <algorithm>

namespace erbsland::cterm::impl::paragraph {

auto LayoutSemantics::hasSoftBreak(const BlockIndex index) const noexcept -> bool {
    return std::ranges::binary_search(softBreaks, index);
}

auto LayoutSemantics::indivisibleRangeAt(const BlockIndex index) const noexcept -> std::optional<BlockRange> {
    const auto iterator = std::ranges::lower_bound(
        indivisibleRanges, index, {}, [](const BlockRange &range) noexcept { return range.index(); });
    if (iterator == indivisibleRanges.end() || iterator->index() != index) {
        return std::nullopt;
    }
    return *iterator;
}

}
