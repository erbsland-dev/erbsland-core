// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../String.hpp"

#include "../../unit/ColumnIndex.hpp"
#include "../../unit/ColumnRange.hpp"

#include <cstddef>
#include <optional>

namespace erbsland::text::impl {

/// One terminal-displayable source cell in a laid-out code snippet row.
struct CodeSnippetLayoutCell final {
    String text;                       ///< The printable source slice or replacement text.
    unit::ColumnRange range;           ///< Represented logical source columns.
    int width{1};                      ///< Terminal-cell width.
    std::optional<std::size_t> marker; ///< Last marker affecting this cell.
    bool isEllipsis{false};            ///< `true` if this cell represents cropped source text.

    /// Create a source ellipsis cell.
    [[nodiscard]] static auto ellipsis() -> CodeSnippetLayoutCell;
    /// Test whether this cell overlaps a non-empty logical source range.
    [[nodiscard]] auto overlaps(unit::ColumnRange other) const noexcept -> bool;
    /// Test whether a point belongs to or precedes this cell.
    [[nodiscard]] auto containsOrFollows(unit::ColumnIndex point) const noexcept -> bool;
};

}
