// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BlockPosition.hpp"

namespace erbsland::bgeo {

void BlockPosition::setX(const BlockCoordinate x) noexcept {
    _x = x;
}

void BlockPosition::setY(const BlockCoordinate y) noexcept {
    _y = y;
}

auto BlockPosition::distanceTo(const BlockPosition other) const noexcept -> BlockCoordinate {
    return (_x - other._x).toAbsolute() + (_y - other._y).toAbsolute();
}

auto BlockPosition::cardinalFourDeltas() noexcept -> const std::array<BlockPosition, 4> & {
    static const std::array<BlockPosition, 4> deltas = {{{1, 0}, {0, 1}, {-1, 0}, {0, -1}}};
    return deltas;
}

auto BlockPosition::ringEightDeltas() noexcept -> const std::array<BlockPosition, 8U> & {
    static const std::array<BlockPosition, 8U> deltas = {{
        {1, 0},
        {1, 1},
        {0, 1},
        {-1, 1},
        {-1, 0},
        {-1, -1},
        {0, -1},
        {1, -1},
    }};
    return deltas;
}

}
