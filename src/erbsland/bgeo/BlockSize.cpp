// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BlockSize.hpp"

namespace erbsland::bgeo {

auto BlockSize::anchor(const BlockAnchor anchor) const noexcept -> BlockPosition {
    auto x = BlockCoordinate{0};
    auto y = BlockCoordinate{0};
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
    return BlockPosition{x, y};
}

auto BlockSize::alignmentOffset(const BlockSize contentSize, const Alignment alignment) const noexcept
    -> BlockPosition {
    return {
        alignment.horizontalOffset(_width, contentSize.width()),
        alignment.verticalOffset(_height, contentSize.height())};
}

auto BlockSize::rotateCCW(const BlockPosition &pos, int rotation) const noexcept -> BlockPosition {
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

auto BlockSize::mirror(const BlockPosition &pos, Orientation orientation) const noexcept -> BlockPosition {
    if (orientation == Orientation::Horizontal) {
        return {_width - 1 - pos.x(), pos.y()};
    }
    return {pos.x(), _height - 1 - pos.y()};
}

auto BlockSize::transform(const BlockPosition &pos, Symmetry symmetry) const noexcept -> BlockPosition {
    switch (symmetry) {
    case Symmetry::Identity:
        return pos;
    case Symmetry::Rotate90:
        return rotateCCW(pos, 1);
    case Symmetry::Rotate180:
        return rotateCCW(pos, 2);
    case Symmetry::Rotate270:
        return rotateCCW(pos, 3);
    case Symmetry::MirrorHorizontal:
        return mirror(pos, Orientation::Horizontal);
    case Symmetry::MirrorVertical:
        return mirror(pos, Orientation::Vertical);
    case Symmetry::MirrorDiagonal:
        return {pos.y(), pos.x()};
    case Symmetry::MirrorAntiDiagonal:
        return {_height - 1 - pos.y(), _width - 1 - pos.x()};
    }
    return pos;
}

}
