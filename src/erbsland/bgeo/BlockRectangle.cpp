// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BlockRectangle.hpp"

#include "BlockAlignedSource.hpp"

#include "../err/ParameterError.hpp"

namespace erbsland::bgeo {

auto BlockRectangle::operator|(const BlockRectangle &other) const noexcept -> BlockRectangle {
    return BlockRectangle{topLeft().componentMin(other.topLeft()), bottomRight().componentMax(other.bottomRight())};
}

auto BlockRectangle::operator|=(const BlockRectangle &other) noexcept -> BlockRectangle & {
    *this = *this | other;
    return *this;
}

auto BlockRectangle::operator&(const BlockRectangle &other) const noexcept -> BlockRectangle {
    if (!overlaps(other)) {
        return BlockRectangle{};
    }
    const auto newTopLeft = topLeft().componentMax(other.topLeft());
    const auto newBottomRight = bottomRight().componentMin(other.bottomRight());
    return BlockRectangle{newTopLeft, newBottomRight};
}

auto BlockRectangle::operator&=(const BlockRectangle &other) noexcept -> BlockRectangle & {
    *this = *this & other;
    return *this;
}

auto BlockRectangle::anchor(const BlockAnchor anchor) const noexcept -> BlockPosition {
    return _pos + _size.anchor(anchor);
}

auto BlockRectangle::expandedBy(const BlockMargins margins) const noexcept -> BlockRectangle {
    return BlockRectangle{
        _pos.x() - margins.left(),
        _pos.y() - margins.top(),
        _size.width() + margins.left() + margins.right(),
        _size.height() + margins.top() + margins.bottom()};
}

auto BlockRectangle::insetBy(const BlockMargins margins) const noexcept -> BlockRectangle {
    return expandedBy(-margins);
}

auto BlockRectangle::subRectangle(const BlockAnchor anchor, BlockSize size, const BlockMargins margins) const noexcept
    -> BlockRectangle {
    const auto innerRect = insetBy(margins);
    size = size.limitedWith(innerRect.size());
    if (size.width() == 0) {
        size.setWidth(innerRect.width());
    }
    if (size.height() == 0) {
        size.setHeight(innerRect.height());
    }
    auto topLeftRect = BlockRectangle(innerRect.topLeft(), size.limitedWith(innerRect.size()));
    const auto offset = innerRect.anchor(anchor) - topLeftRect.anchor(anchor);
    return BlockRectangle{innerRect.topLeft() + offset, size};
}

auto BlockRectangle::alignmentOffset(const BlockSize contentSize, const Alignment alignment) const noexcept
    -> BlockPosition {
    return topLeft() + size().alignmentOffset(contentSize, alignment);
}

auto BlockRectangle::alignedSource(BlockRectangle sourceRect, const Alignment alignment) const noexcept
    -> BlockAlignedSource {
    auto targetRect = *this;
    const auto originalSourceSize = sourceRect.size();
    const auto originalTargetSize = targetRect.size();
    const auto localOffset = size().alignmentOffset(originalSourceSize, alignment);
    const auto targetOffset = topLeft() + localOffset;
    const auto visibleSize = originalSourceSize.limitedWith(originalTargetSize);
    sourceRect.setSize(visibleSize);
    targetRect.setSize(visibleSize);
    if (originalSourceSize.width() <= originalTargetSize.width()) {
        targetRect.setPos(BlockPosition{targetOffset.x(), targetRect.y1()});
    } else {
        sourceRect.setPos(BlockPosition{sourceRect.x1() - localOffset.x(), sourceRect.y1()});
    }
    if (originalSourceSize.height() <= originalTargetSize.height()) {
        targetRect.setPos(BlockPosition{targetRect.x1(), targetOffset.y()});
    } else {
        sourceRect.setPos(BlockPosition{sourceRect.x1(), sourceRect.y1() - localOffset.y()});
    }
    return {targetRect, sourceRect};
}

auto BlockRectangle::contains(const BlockPosition testedPosition) const noexcept -> bool {
    return testedPosition.x() >= _pos.x() && testedPosition.y() >= _pos.y() && testedPosition.x() < x2() &&
        testedPosition.y() < y2();
}

auto BlockRectangle::contains(const BlockRectangle testedRectangle) const noexcept -> bool {
    return testedRectangle.x1() >= x1() && testedRectangle.x2() <= x2() && testedRectangle.y1() >= y1() &&
        testedRectangle.y2() <= y2();
}

auto BlockRectangle::overlaps(const BlockRectangle testedRectangle) const noexcept -> bool {
    return x1() < testedRectangle.x2() && x2() > testedRectangle.x1() && y1() < testedRectangle.y2() &&
        y2() > testedRectangle.y1();
}

auto BlockRectangle::isFrame(const BlockPosition testedPosition) const noexcept -> bool {
    return contains(testedPosition) &&
        (testedPosition.x() == _pos.x() || testedPosition.y() == _pos.y() || testedPosition.x() == x2() - 1 ||
            testedPosition.y() == y2() - 1);
}

auto BlockRectangle::clamp(BlockPosition position) const noexcept -> BlockPosition {
    const auto maximumX = width() > BlockCoordinate{0} ? x2() - 1 : x1();
    const auto maximumY = height() > BlockCoordinate{0} ? y2() - 1 : y1();
    return BlockPosition{position.x().clamped(x1(), maximumX), position.y().clamped(y1(), maximumY)};
}

auto BlockRectangle::frameDirection(const BlockPosition testedPosition) const noexcept -> BlockDirection {
    if (!isFrame(testedPosition)) {
        return BlockDirection::None;
    }
    // We can't reliably return a frame direction for 1x1 or degenerate rectangles.
    if (width() < 2 || height() < 2) {
        return BlockDirection::None;
    }

    const bool north = testedPosition.y() == y1();
    const bool east = testedPosition.x() == x2() - 1;
    const bool south = testedPosition.y() == y2() - 1;
    const bool west = testedPosition.x() == x1();

    if (north && west) {
        return BlockDirection::NorthWest;
    }
    if (north && east) {
        return BlockDirection::NorthEast;
    }
    if (south && east) {
        return BlockDirection::SouthEast;
    }
    if (south && west) {
        return BlockDirection::SouthWest;
    }
    if (north) {
        return BlockDirection::North;
    }
    if (east) {
        return BlockDirection::East;
    }
    if (south) {
        return BlockDirection::South;
    }
    return BlockDirection::West;
}

auto BlockRectangle::frameIndex(const BlockPosition testedPosition) const noexcept -> int64_t {
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

auto BlockRectangle::gridCells(
    const int rows,
    const int columns,
    const BlockCoordinate horizontalSpacing,
    const BlockCoordinate verticalSpacing) const -> std::vector<BlockRectangle> {
    if (rows < 1) {
        throw err::ParameterError{"BlockRect::gridCells() requires at least one row.", "rows"};
    }
    if (columns < 1) {
        throw err::ParameterError{"BlockRect::gridCells() requires at least one column.", "columns"};
    }
    if (horizontalSpacing < 0 || verticalSpacing < 0) {
        throw err::ParameterError{
            "BlockRect::gridCells() spacing must not be negative.", "horizontalSpacing/verticalSpacing"};
    }
    const auto usableWidth = width() - horizontalSpacing * BlockCoordinate{columns - 1};
    const auto usableHeight = height() - verticalSpacing * BlockCoordinate{rows - 1};
    if (usableWidth < columns || usableHeight < rows) {
        throw err::ParameterError{"BlockRect::gridCells() cannot create cells of at least 1x1.", "rows/columns"};
    }
    const auto baseCellWidth = usableWidth / columns;
    const auto extraWidthCells = usableWidth % columns;
    const auto baseCellHeight = usableHeight / rows;
    const auto extraHeightCells = usableHeight % rows;
    auto result = std::vector<BlockRectangle>{};
    result.reserve(static_cast<std::size_t>(rows) * static_cast<std::size_t>(columns));
    auto y = y1();
    for (int row = 0; row < rows; ++row) {
        const auto cellHeight =
            baseCellHeight + (BlockCoordinate{row} < extraHeightCells ? BlockCoordinate{1} : BlockCoordinate{0});
        auto x = x1();
        for (int column = 0; column < columns; ++column) {
            const auto cellWidth =
                baseCellWidth + (BlockCoordinate{column} < extraWidthCells ? BlockCoordinate{1} : BlockCoordinate{0});
            result.emplace_back(x, y, cellWidth, cellHeight);
            x += cellWidth + horizontalSpacing;
        }
        y += cellHeight + verticalSpacing;
    }
    return result;
}

auto BlockRectangle::rotateCCW(const BlockPosition &pos, int rotation) const noexcept -> BlockPosition {
    return _pos + _size.rotateCCW(pos - _pos, rotation);
}

auto BlockRectangle::mirror(const BlockPosition &pos, Orientation orientation) const noexcept -> BlockPosition {
    return _pos + _size.mirror(pos - _pos, orientation);
}

auto BlockRectangle::transform(const BlockPosition &pos, Symmetry symmetry) const noexcept -> BlockPosition {
    return _pos + _size.transform(pos - _pos, symmetry);
}

auto BlockRectangle::bounds(const BlockPositionList &positions) noexcept -> BlockRectangle {
    if (positions.isEmpty()) {
        return BlockRectangle{};
    }
    auto topLeft = BlockPosition::maximum();
    auto bottomRight = BlockPosition::minimum();
    for (const auto &position : positions) {
        topLeft = topLeft.componentMin(position);
        bottomRight = bottomRight.componentMax(position);
    }
    return BlockRectangle{topLeft, bottomRight + BlockPosition{1, 1}};
}

}
