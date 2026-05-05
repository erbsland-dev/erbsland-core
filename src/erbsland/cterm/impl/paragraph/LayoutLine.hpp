// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LayoutFragments.hpp"

namespace erbsland::cterm::impl::paragraph {

/// One rendered physical line in the final paragraph layout.
class LayoutLine final {
public:
    LayoutLine() = default;
    ~LayoutLine() = default;
    LayoutLine(const LayoutLine &) = delete;
    LayoutLine(LayoutLine &&) noexcept = default;
    auto operator=(const LayoutLine &) -> LayoutLine & = delete;
    auto operator=(LayoutLine &&) noexcept -> LayoutLine & = default;

public:
    /// Get the display width of the rendered text including indentation.
    /// @return The display width of the rendered text including indentation.
    [[nodiscard]] auto textWidth() const noexcept -> int { return indentWidth + fragments.width(); }

public:
    LayoutFragments fragments;        ///< The rendered line text without the end mark aligned to the right edge.
    int indentWidth = 0;              ///< Width of the indentation area before the first rendered mark/text.
    bool wrappedFromPrevious = false; ///< `true` if this line is a wrapped continuation.
    bool wrapsToNext = false;         ///< `true` if this line wraps to the next physical line.
};

}
