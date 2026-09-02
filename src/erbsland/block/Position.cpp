// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Position.hpp"

namespace erbsland::block {

void Position::setX(const Coordinate x) noexcept {
    _x = x;
}

void Position::setY(const Coordinate y) noexcept {
    _y = y;
}

auto Position::distanceTo(const Position other) const noexcept -> Coordinate {
    return (_x - other._x).toAbsolute() + (_y - other._y).toAbsolute();
}

auto Position::cardinalFourDeltas() noexcept -> const std::array<Position, 4> & {
    static const std::array<Position, 4> deltas = {{{1, 0}, {0, 1}, {-1, 0}, {0, -1}}};
    return deltas;
}

auto Position::ringEightDeltas() noexcept -> const std::array<Position, 8U> & {
    static const std::array<Position, 8U> deltas = {{
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
