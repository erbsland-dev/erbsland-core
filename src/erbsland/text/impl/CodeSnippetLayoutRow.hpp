// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CodeSnippetLayoutCell.hpp"

#include "../../unit/ColumnRange.hpp"

#include <optional>
#include <utility>
#include <vector>

namespace erbsland::text::impl {

/// One physical source row without a gutter or marker annotation.
/// @tested{CodeSnippetLayoutTest}
class CodeSnippetLayoutRow final {
public:
    /// Display placement for a visible marker.
    /// @tested{CodeSnippetLayoutTest}
    struct MarkerPlacement final {
        int start{};  ///< Display-cell offset from the start of the row.
        int length{}; ///< Display-cell length of the marker.
    };

public:
    /// Create an empty row.
    CodeSnippetLayoutRow() = default;
    /// Create a row from visible source cells.
    explicit CodeSnippetLayoutRow(std::vector<CodeSnippetLayoutCell> cells) : _cells{std::move(cells)} {}

public:
    /// Access the visible source cells.
    [[nodiscard]] auto cells() const noexcept -> const std::vector<CodeSnippetLayoutCell> & { return _cells; }
    /// Get the display width including crop ellipses.
    [[nodiscard]] auto displayWidth() const noexcept -> int;
    /// Locate a marker in this row.
    /// @param range The logical point or column range.
    /// @return Its visible placement, or no value if it does not intersect this row.
    [[nodiscard]] auto markerPlacement(unit::ColumnRange range) const noexcept -> std::optional<MarkerPlacement>;
    /// Crop source cells from the leading edge and insert an ellipsis.
    /// @param width The maximum display width.
    void cropLeading(int width);
    /// Crop source cells from the trailing edge and append an ellipsis.
    /// @param width The maximum display width.
    void cropTrailing(int width);

private:
    /// Crop one edge of this row and insert the corresponding ellipsis.
    void crop(int width, bool leading);

private:
    std::vector<CodeSnippetLayoutCell> _cells; ///< The visible source cells.
};

}
