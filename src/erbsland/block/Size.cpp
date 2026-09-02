// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Size.hpp"

namespace erbsland::block {

auto Size::anchor(const geometry::Anchor anchor) const noexcept -> Position {
    auto x = Coordinate{0};
    auto y = Coordinate{0};
    if (anchor.isHorizontalCenter()) {
        x = halfWidthForPosition();
    } else if (anchor.isRight()) {
        x = widthForPosition();
    }
    if (anchor.isVerticalCenter()) {
        y = halfHeightForPosition();
    } else if (anchor.isBottom()) {
        y = heightForPosition();
    }
    return Position{x, y};
}

auto Size::alignmentOffset(const Size contentSize, const geometry::Alignment alignment) const noexcept -> Position {
    return {
        horizontalAlignmentOffset(_width, contentSize.width(), alignment),
        verticalAlignmentOffset(_height, contentSize.height(), alignment)};
}

auto Size::horizontalAlignmentOffset(
    const Coordinate availableWidth, const Coordinate contentWidth, const geometry::Alignment alignment) noexcept
    -> Coordinate {
    const auto freeSpace = availableWidth - contentWidth;
    if (alignment.isHorizontalCenter()) {
        return freeSpace / 2;
    }
    return alignment.isRight() ? freeSpace : Coordinate{0};
}

auto Size::verticalAlignmentOffset(
    const Coordinate availableHeight, const Coordinate contentHeight, const geometry::Alignment alignment) noexcept
    -> Coordinate {
    const auto freeSpace = availableHeight - contentHeight;
    if (alignment.isVerticalCenter()) {
        return freeSpace / 2;
    }
    return alignment.isBottom() ? freeSpace : Coordinate{0};
}

auto Size::rotateCCW(const Position &pos, int rotation) const noexcept -> Position {
    rotation %= 4;
    if (rotation < 0) {
        rotation += 4;
    }
    switch (rotation) {
    case 1:
        return {pos.y(), _width - 1 - pos.x()};
    case 2:
        return {_width - 1 - pos.x(), _height - 1 - pos.y()};
    case 3:
        return {_height - 1 - pos.y(), pos.x()};
    default:
        return pos;
    }
}

auto Size::mirror(const Position &pos, geometry::Orientation orientation) const noexcept -> Position {
    if (orientation == geometry::Orientation::Horizontal) {
        return {_width - 1 - pos.x(), pos.y()};
    }
    return {pos.x(), _height - 1 - pos.y()};
}

auto Size::transform(const Position &pos, geometry::Symmetry symmetry) const noexcept -> Position {
    switch (symmetry) {
    case geometry::Symmetry::Identity:
        return pos;
    case geometry::Symmetry::Rotate90:
        return rotateCCW(pos, 1);
    case geometry::Symmetry::Rotate180:
        return rotateCCW(pos, 2);
    case geometry::Symmetry::Rotate270:
        return rotateCCW(pos, 3);
    case geometry::Symmetry::MirrorHorizontal:
        return mirror(pos, geometry::Orientation::Horizontal);
    case geometry::Symmetry::MirrorVertical:
        return mirror(pos, geometry::Orientation::Vertical);
    case geometry::Symmetry::MirrorDiagonal:
        return {pos.y(), pos.x()};
    case geometry::Symmetry::MirrorAntiDiagonal:
        return {_height - 1 - pos.y(), _width - 1 - pos.x()};
    }
    return pos;
}

}
