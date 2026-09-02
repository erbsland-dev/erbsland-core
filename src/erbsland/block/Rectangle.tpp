// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <concepts>

namespace erbsland::block {

template <typename Fn>
void Rectangle::forEach(Fn fn) const {
    for (Coordinate y = Coordinate{0}; y < _size.height(); ++y) {
        for (Coordinate x = Coordinate{0}; x < _size.width(); ++x) {
            fn(Position{x, y} + _pos);
        }
    }
}

template <typename Fn>
    requires(std::is_invocable_r_v<void, Fn, Position, int> || std::is_invocable_r_v<void, Fn, Position>)
void Rectangle::forEachInFrame(Fn fn) const {
    if (_size.width() <= Coordinate{0} || _size.height() <= Coordinate{0}) {
        return;
    }
    if constexpr (std::is_invocable_r_v<void, Fn, Position, int>) {
        int index = 0;
        for (auto x = Coordinate{0}; x < _size.width(); ++x) {
            fn(Position{x1() + x, y1()}, index++);
        }
        if (_size.height() > Coordinate{1}) {
            for (auto y = Coordinate{1}; y < _size.height(); ++y) {
                fn(Position{x2() - 1, y1() + y}, index++);
            }
            for (auto x = _size.width() - Coordinate{2}; x >= Coordinate{0}; --x) {
                fn(Position{x1() + x, y2() - 1}, index++);
            }
            if (_size.width() > Coordinate{1}) {
                for (auto y = _size.height() - Coordinate{2}; y > Coordinate{0}; --y) {
                    fn(Position{x1(), y1() + y}, index++);
                }
            }
        }
    } else {
        for (auto x = Coordinate{0}; x < _size.width(); ++x) {
            fn(Position{x1() + x, y1()});
        }
        if (_size.height() > Coordinate{1}) {
            for (auto y = Coordinate{1}; y < _size.height(); ++y) {
                fn(Position{x2() - 1, y1() + y});
            }
            for (auto x = _size.width() - Coordinate{2}; x >= Coordinate{0}; --x) {
                fn(Position{x1() + x, y2() - 1});
            }
            if (_size.width() > Coordinate{1}) {
                for (auto y = _size.height() - Coordinate{2}; y > Coordinate{0}; --y) {
                    fn(Position{x1(), y1() + y});
                }
            }
        }
    }
}

}
