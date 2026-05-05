// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "GridLayout.hpp"

#include <numeric>
#include <stdexcept>
#include <string>
#include <utility>

namespace erbsland::cterm {

GridLayout::GridLayout(std::vector<bgeo::BlockCoordinate> columnWidths, std::vector<bgeo::BlockCoordinate> rowHeights) :
    _columnWidths{std::move(columnWidths)}, _rowHeights{std::move(rowHeights)} {

    validateSizes(_columnWidths, "columnWidths");
    validateSizes(_rowHeights, "rowHeights");
}

GridLayout::GridLayout(
    const std::initializer_list<bgeo::BlockCoordinate> columnWidths,
    const std::initializer_list<bgeo::BlockCoordinate> rowHeights) :
    GridLayout{std::vector<bgeo::BlockCoordinate>{columnWidths}, std::vector<bgeo::BlockCoordinate>{rowHeights}} {
}

auto GridLayout::rowCount() const noexcept -> std::size_t {
    return _rowHeights.size();
}

auto GridLayout::columnCount() const noexcept -> std::size_t {
    return _columnWidths.size();
}

auto GridLayout::rowHeight(const std::size_t row) const -> bgeo::BlockCoordinate {
    return _rowHeights.at(row);
}

auto GridLayout::columnWidth(const std::size_t column) const -> bgeo::BlockCoordinate {
    return _columnWidths.at(column);
}

auto GridLayout::rowHeights() const noexcept -> const std::vector<bgeo::BlockCoordinate> & {
    return _rowHeights;
}

auto GridLayout::columnWidths() const noexcept -> const std::vector<bgeo::BlockCoordinate> & {
    return _columnWidths;
}

auto GridLayout::size(const FrameBorder &border) const noexcept -> bgeo::BlockSize {
    const auto contentWidth = std::accumulate(_columnWidths.begin(), _columnWidths.end(), bgeo::BlockCoordinate{0});
    const auto contentHeight = std::accumulate(_rowHeights.begin(), _rowHeights.end(), bgeo::BlockCoordinate{0});
    const auto separatorWidth =
        bgeo::BlockCoordinate{columnCount() - 1} * borderSize(border, FrameBorder::Element::VLine);
    const auto separatorHeight =
        bgeo::BlockCoordinate{rowCount() - 1} * borderSize(border, FrameBorder::Element::HLine);
    return bgeo::BlockSize{
        contentWidth + separatorWidth + borderSize(border, FrameBorder::Element::Left) +
            borderSize(border, FrameBorder::Element::Right),
        contentHeight + separatorHeight + borderSize(border, FrameBorder::Element::Top) +
            borderSize(border, FrameBorder::Element::Bottom)};
}

auto GridLayout::cellRect(
    const std::size_t row, const std::size_t column, const bgeo::BlockPosition origin, const FrameBorder &border) const
    -> bgeo::BlockRectangle {

    if (row >= rowCount() || column >= columnCount()) {
        throw std::out_of_range{"GridLayout cell index is out of range."};
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
    return bgeo::BlockRectangle{x, y, _columnWidths[column], _rowHeights[row]};
}

auto GridLayout::borderSize(const FrameBorder &border, const FrameBorder::Element element) noexcept
    -> bgeo::BlockCoordinate {
    const auto style = border.style(element);
    return bgeo::BlockCoordinate{style != FrameStyle::None && FrameBorder::isLineStyle(style) ? 1 : 0};
}

void GridLayout::validateSizes(const std::vector<bgeo::BlockCoordinate> &sizes, const std::string_view name) {
    if (sizes.empty()) {
        throw std::invalid_argument{std::string{name} + " must not be empty."};
    }
    for (const auto size : sizes) {
        if (size <= 0) {
            throw std::invalid_argument{std::string{name} + " must contain only positive sizes."};
        }
    }
}

}
