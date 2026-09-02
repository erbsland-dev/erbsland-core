// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Rectangle.hpp"

#include "AlignedSource.hpp"

#include "../err/ParameterError.hpp"
#include "../text/Literals.hpp"

namespace erbsland::block {

using namespace text::literals;

auto Rectangle::operator|(const Rectangle &other) const noexcept -> Rectangle {
    return Rectangle{topLeft().componentMin(other.topLeft()), bottomRight().componentMax(other.bottomRight())};
}

auto Rectangle::operator|=(const Rectangle &other) noexcept -> Rectangle & {
    *this = *this | other;
    return *this;
}

auto Rectangle::operator&(const Rectangle &other) const noexcept -> Rectangle {
    if (!overlaps(other)) {
        return Rectangle{};
    }
    const auto newTopLeft = topLeft().componentMax(other.topLeft());
    const auto newBottomRight = bottomRight().componentMin(other.bottomRight());
    return Rectangle{newTopLeft, newBottomRight};
}

auto Rectangle::operator&=(const Rectangle &other) noexcept -> Rectangle & {
    *this = *this & other;
    return *this;
}

auto Rectangle::anchor(const geometry::Anchor anchor) const noexcept -> Position {
    return _pos + _size.anchor(anchor);
}

auto Rectangle::expandedBy(const Margins margins) const noexcept -> Rectangle {
    const auto horizontal = margins.horizontal();
    const auto vertical = margins.vertical();
    return Rectangle{
        _pos.x() - horizontal.leading(),
        _pos.y() - vertical.leading(),
        _size.width() + horizontal.delta(),
        _size.height() + vertical.delta()};
}

auto Rectangle::insetBy(const Margins margins) const noexcept -> Rectangle {
    return expandedBy(-margins);
}

auto Rectangle::subRectangle(const geometry::Anchor anchor, Size size, const Margins margins) const noexcept
    -> Rectangle {
    const auto innerRect = insetBy(margins);
    size = size.limitedWith(innerRect.size());
    if (size.width() == 0) {
        size.setWidth(innerRect.width());
    }
    if (size.height() == 0) {
        size.setHeight(innerRect.height());
    }
    auto topLeftRect = Rectangle(innerRect.topLeft(), size.limitedWith(innerRect.size()));
    const auto offset = innerRect.anchor(anchor) - topLeftRect.anchor(anchor);
    return Rectangle{innerRect.topLeft() + offset, size};
}

auto Rectangle::alignmentOffset(const Size contentSize, const geometry::Alignment alignment) const noexcept
    -> Position {
    return topLeft() + size().alignmentOffset(contentSize, alignment);
}

auto Rectangle::alignedSource(Rectangle sourceRect, const geometry::Alignment alignment) const noexcept
    -> AlignedSource {
    auto targetRect = *this;
    const auto originalSourceSize = sourceRect.size();
    const auto originalTargetSize = targetRect.size();
    const auto localOffset = size().alignmentOffset(originalSourceSize, alignment);
    const auto targetOffset = topLeft() + localOffset;
    const auto visibleSize = originalSourceSize.limitedWith(originalTargetSize);
    sourceRect.setSize(visibleSize);
    targetRect.setSize(visibleSize);
    if (originalSourceSize.width() <= originalTargetSize.width()) {
        targetRect.setPos(Position{targetOffset.x(), targetRect.y1()});
    } else {
        sourceRect.setPos(Position{sourceRect.x1() - localOffset.x(), sourceRect.y1()});
    }
    if (originalSourceSize.height() <= originalTargetSize.height()) {
        targetRect.setPos(Position{targetRect.x1(), targetOffset.y()});
    } else {
        sourceRect.setPos(Position{sourceRect.x1(), sourceRect.y1() - localOffset.y()});
    }
    return {targetRect, sourceRect};
}

auto Rectangle::contains(const Position testedPosition) const noexcept -> bool {
    return testedPosition.x() >= _pos.x() && testedPosition.y() >= _pos.y() && testedPosition.x() < x2() &&
        testedPosition.y() < y2();
}

auto Rectangle::contains(const Rectangle testedRectangle) const noexcept -> bool {
    return testedRectangle.x1() >= x1() && testedRectangle.x2() <= x2() && testedRectangle.y1() >= y1() &&
        testedRectangle.y2() <= y2();
}

auto Rectangle::overlaps(const Rectangle testedRectangle) const noexcept -> bool {
    return x1() < testedRectangle.x2() && x2() > testedRectangle.x1() && y1() < testedRectangle.y2() &&
        y2() > testedRectangle.y1();
}

auto Rectangle::isFrame(const Position testedPosition) const noexcept -> bool {
    return contains(testedPosition) &&
        (testedPosition.x() == _pos.x() || testedPosition.y() == _pos.y() || testedPosition.x() == x2() - 1 ||
            testedPosition.y() == y2() - 1);
}

auto Rectangle::clamp(Position position) const noexcept -> Position {
    const auto maximumX = width() > Coordinate{0} ? x2() - 1 : x1();
    const auto maximumY = height() > Coordinate{0} ? y2() - 1 : y1();
    return Position{position.x().clamped(x1(), maximumX), position.y().clamped(y1(), maximumY)};
}

auto Rectangle::frameDirection(const Position testedPosition) const noexcept -> Direction {
    if (!isFrame(testedPosition)) {
        return Direction::None;
    }
    // We can't reliably return a frame direction for 1x1 or degenerate rectangles.
    if (width() < 2 || height() < 2) {
        return Direction::None;
    }

    const bool north = testedPosition.y() == y1();
    const bool east = testedPosition.x() == x2() - 1;
    const bool south = testedPosition.y() == y2() - 1;
    const bool west = testedPosition.x() == x1();

    if (north && west) {
        return Direction::NorthWest;
    }
    if (north && east) {
        return Direction::NorthEast;
    }
    if (south && east) {
        return Direction::SouthEast;
    }
    if (south && west) {
        return Direction::SouthWest;
    }
    if (north) {
        return Direction::North;
    }
    if (east) {
        return Direction::East;
    }
    if (south) {
        return Direction::South;
    }
    return Direction::West;
}

auto Rectangle::frameIndex(const Position testedPosition) const noexcept -> int64_t {
    if (!isFrame(testedPosition)) {
        return -1;
    }
    if (width() <= 1) {
        return (testedPosition.y() - y1()).toRawValue();
    }
    if (height() <= 1) {
        return (testedPosition.x() - x1()).toRawValue();
    }
    if (testedPosition.y() == y1()) {
        return (testedPosition.x() - x1()).toRawValue();
    }
    if (testedPosition.x() == x2() - 1) {
        return ((width() - 1) + (testedPosition.y() - y1())).toRawValue();
    }
    if (testedPosition.y() == y2() - 1) {
        return ((width() - 1) + (height() - 1) + (x2() - 1 - testedPosition.x())).toRawValue();
    }
    return ((width() - 1) + (width() - 1) + (height() - 1) + (y2() - 1 - testedPosition.y())).toRawValue();
}

auto Rectangle::gridCells(
    const int rows, const int columns, const Coordinate horizontalSpacing, const Coordinate verticalSpacing) const
    -> std::vector<Rectangle> {
    if (rows < 1) {
        throw err::ParameterError{"BlockRect::gridCells() requires at least one row."_el, "rows"_el};
    }
    if (columns < 1) {
        throw err::ParameterError{"BlockRect::gridCells() requires at least one column."_el, "columns"_el};
    }
    if (horizontalSpacing < 0 || verticalSpacing < 0) {
        throw err::ParameterError{
            "BlockRect::gridCells() spacing must not be negative."_el, "horizontalSpacing/verticalSpacing"_el};
    }
    const auto usableWidth = width() - horizontalSpacing * Coordinate{columns - 1};
    const auto usableHeight = height() - verticalSpacing * Coordinate{rows - 1};
    if (usableWidth < columns || usableHeight < rows) {
        throw err::ParameterError{"BlockRect::gridCells() cannot create cells of at least 1x1."_el, "rows/columns"_el};
    }
    const auto baseCellWidth = usableWidth / columns;
    const auto extraWidthCells = usableWidth % columns;
    const auto baseCellHeight = usableHeight / rows;
    const auto extraHeightCells = usableHeight % rows;
    auto result = std::vector<Rectangle>{};
    result.reserve(static_cast<std::size_t>(rows) * static_cast<std::size_t>(columns));
    auto y = y1();
    for (int row = 0; row < rows; ++row) {
        const auto cellHeight = baseCellHeight + (Coordinate{row} < extraHeightCells ? Coordinate{1} : Coordinate{0});
        auto x = x1();
        for (int column = 0; column < columns; ++column) {
            const auto cellWidth =
                baseCellWidth + (Coordinate{column} < extraWidthCells ? Coordinate{1} : Coordinate{0});
            result.emplace_back(x, y, cellWidth, cellHeight);
            x += cellWidth + horizontalSpacing;
        }
        y += cellHeight + verticalSpacing;
    }
    return result;
}

auto Rectangle::rotateCCW(const Position &pos, int rotation) const noexcept -> Position {
    return _pos + _size.rotateCCW(pos - _pos, rotation);
}

auto Rectangle::mirror(const Position &pos, geometry::Orientation orientation) const noexcept -> Position {
    return _pos + _size.mirror(pos - _pos, orientation);
}

auto Rectangle::transform(const Position &pos, geometry::Symmetry symmetry) const noexcept -> Position {
    return _pos + _size.transform(pos - _pos, symmetry);
}

auto Rectangle::bounds(const PositionList &positions) noexcept -> Rectangle {
    if (positions.isEmpty()) {
        return Rectangle{};
    }
    auto topLeft = Position::maximum();
    auto bottomRight = Position::minimum();
    for (const auto &position : positions) {
        topLeft = topLeft.componentMin(position);
        bottomRight = bottomRight.componentMax(position);
    }
    return Rectangle{topLeft, bottomRight + Position{1, 1}};
}

}
