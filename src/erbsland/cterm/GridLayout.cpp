// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "GridLayout.hpp"

#include "../err/OutOfRangeError.hpp"
#include "../err/ParameterError.hpp"
#include "../text/Literals.hpp"

#include <numeric>
#include <utility>

namespace erbsland::cterm {

using block::Coordinate;
using block::Position;
using block::Rectangle;
using block::Size;
using namespace text::literals;

GridLayout::GridLayout(std::vector<Coordinate> columnWidths, std::vector<Coordinate> rowHeights) :
    _columnWidths{std::move(columnWidths)}, _rowHeights{std::move(rowHeights)} {

    validateSizes(_columnWidths, "columnWidths"_el);
    validateSizes(_rowHeights, "rowHeights"_el);
}

GridLayout::GridLayout(
    const std::initializer_list<Coordinate> columnWidths, const std::initializer_list<Coordinate> rowHeights) :
    GridLayout{std::vector<Coordinate>{columnWidths}, std::vector<Coordinate>{rowHeights}} {
}

auto GridLayout::rowCount() const noexcept -> std::size_t {
    return _rowHeights.size();
}

auto GridLayout::columnCount() const noexcept -> std::size_t {
    return _columnWidths.size();
}

auto GridLayout::rowHeight(const std::size_t row) const -> Coordinate {
    if (row >= rowCount()) {
        throw err::OutOfRangeError{"GridLayout row index is out of range."_el};
    }
    return _rowHeights[row];
}

auto GridLayout::columnWidth(const std::size_t column) const -> Coordinate {
    if (column >= columnCount()) {
        throw err::OutOfRangeError{"GridLayout column index is out of range."_el};
    }
    return _columnWidths[column];
}

auto GridLayout::rowHeights() const noexcept -> const std::vector<Coordinate> & {
    return _rowHeights;
}

auto GridLayout::columnWidths() const noexcept -> const std::vector<Coordinate> & {
    return _columnWidths;
}

auto GridLayout::size(const FrameBorder &border) const noexcept -> Size {
    const auto contentWidth = std::accumulate(_columnWidths.begin(), _columnWidths.end(), Coordinate{0});
    const auto contentHeight = std::accumulate(_rowHeights.begin(), _rowHeights.end(), Coordinate{0});
    const auto separatorWidth = Coordinate{columnCount() - 1} * borderSize(border, FrameBorder::Element::VLine);
    const auto separatorHeight = Coordinate{rowCount() - 1} * borderSize(border, FrameBorder::Element::HLine);
    return Size{
        contentWidth + separatorWidth + borderSize(border, FrameBorder::Element::Left) +
            borderSize(border, FrameBorder::Element::Right),
        contentHeight + separatorHeight + borderSize(border, FrameBorder::Element::Top) +
            borderSize(border, FrameBorder::Element::Bottom)};
}

auto GridLayout::cellRect(
    const std::size_t row, const std::size_t column, const Position origin, const FrameBorder &border) const
    -> Rectangle {

    if (row >= rowCount() || column >= columnCount()) {
        throw err::OutOfRangeError{"GridLayout cell index is out of range."_el};
    }
    const auto leftSize = borderSize(border, FrameBorder::Element::Left);
    const auto topSize = borderSize(border, FrameBorder::Element::Top);
    const auto vLineSize = borderSize(border, FrameBorder::Element::VLine);
    const auto hLineSize = borderSize(border, FrameBorder::Element::HLine);
    auto x = origin.x() + leftSize;
    for (std::size_t columnIndex = 0; columnIndex < column; ++columnIndex) {
        x += _columnWidths[columnIndex] + vLineSize;
    }
    auto y = origin.y() + topSize;
    for (std::size_t rowIndex = 0; rowIndex < row; ++rowIndex) {
        y += _rowHeights[rowIndex] + hLineSize;
    }
    return Rectangle{x, y, _columnWidths[column], _rowHeights[row]};
}

auto GridLayout::borderSize(const FrameBorder &border, const FrameBorder::Element element) noexcept -> Coordinate {
    const auto style = border.style(element);
    return Coordinate{style != FrameStyle::None && FrameBorder::isLineStyle(style) ? 1 : 0};
}

void GridLayout::validateSizes(const std::vector<Coordinate> &sizes, const text::StringLiteral &name) {
    if (sizes.empty()) {
        throw err::ParameterError{"The size list must not be empty."_el, name};
    }
    for (const auto size : sizes) {
        if (size <= 0) {
            throw err::ParameterError{"The size list must contain only positive sizes."_el, name};
        }
    }
}

}
