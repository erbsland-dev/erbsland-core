// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FrameBorder.hpp"

#include "../block/Coordinate.hpp"
#include "../block/Position.hpp"
#include "../block/Rectangle.hpp"
#include "../block/Size.hpp"
#include "../text/StringLiteral.hpp"

#include <initializer_list>
#include <vector>

namespace erbsland::cterm {

/// Geometry for a grid of content cells.
///
/// This class stores only content sizes.
/// @tested{GridLayoutTest}
class GridLayout final {
public:
    /// Create a grid layout.
    /// @param columnWidths The content width of each column. Must not be empty; every value must be positive.
    /// @param rowHeights The content height of each row. Must not be empty; every value must be positive.
    GridLayout(std::vector<block::Coordinate> columnWidths, std::vector<block::Coordinate> rowHeights);
    /// Create a grid layout from initializer lists.
    /// @param columnWidths The content width of each column. Must not be empty; every value must be positive.
    /// @param rowHeights The content height of each row. Must not be empty; every value must be positive.
    GridLayout(
        std::initializer_list<block::Coordinate> columnWidths, std::initializer_list<block::Coordinate> rowHeights);

    // defaults
    ~GridLayout() = default;
    GridLayout(const GridLayout &) = default;
    GridLayout(GridLayout &&) noexcept = default;
    auto operator=(const GridLayout &) -> GridLayout & = default;
    auto operator=(GridLayout &&) noexcept -> GridLayout & = default;

public: // operators
    auto operator==(const GridLayout &other) const noexcept -> bool = default;

public: // accessors
    /// Number of rows in the grid.
    [[nodiscard]] auto rowCount() const noexcept -> std::size_t;
    /// Number of columns in the grid.
    [[nodiscard]] auto columnCount() const noexcept -> std::size_t;
    /// Access one row height.
    /// @param row The row index. Must be inside the layout.
    /// @return The row content height.
    [[nodiscard]] auto rowHeight(std::size_t row) const -> block::Coordinate;
    /// Access one column width.
    /// @param column The column index. Must be inside the layout.
    /// @return The column content width.
    [[nodiscard]] auto columnWidth(std::size_t column) const -> block::Coordinate;
    /// Access all row heights.
    [[nodiscard]] auto rowHeights() const noexcept -> const std::vector<block::Coordinate> &;
    /// Access all column widths.
    [[nodiscard]] auto columnWidths() const noexcept -> const std::vector<block::Coordinate> &;

public:
    /// Calculate the complete size of this layout for the given border.
    /// @param border Border styles that decide which frame lines occupy cells.
    /// @return The total grid size including active frame lines.
    [[nodiscard]] auto size(const FrameBorder &border) const noexcept -> block::Size;
    /// Calculate the content rectangle for one cell.
    /// @param row The row index. Must be inside the layout.
    /// @param column The column index. Must be inside the layout.
    /// @param origin The top-left position of the full grid.
    /// @param border Border styles that decide which frame lines occupy cells.
    /// @return The cell content rectangle.
    [[nodiscard]] auto cellRect(
        std::size_t row, std::size_t column, block::Position origin, const FrameBorder &border) const
        -> block::Rectangle;

private:
    /// Get the size contributed by one border element.
    [[nodiscard]] static auto borderSize(const FrameBorder &border, FrameBorder::Element element) noexcept
        -> block::Coordinate;
    /// Validate row or column sizes.
    static void validateSizes(const std::vector<block::Coordinate> &sizes, const text::StringLiteral &name);

private:
    std::vector<block::Coordinate> _columnWidths;
    std::vector<block::Coordinate> _rowHeights;
};

}
