// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <concepts>

namespace erbsland::bgeo {

template <typename Fn>
void BlockRectangle::forEach(Fn fn) const {
    for (BlockCoordinate y = BlockCoordinate{0}; y < _size.height(); ++y) {
        for (BlockCoordinate x = BlockCoordinate{0}; x < _size.width(); ++x) {
            fn(BlockPosition{x, y} + _pos);
        }
    }
}

template <typename Fn>
    requires(std::is_invocable_r_v<void, Fn, BlockPosition, int> || std::is_invocable_r_v<void, Fn, BlockPosition>)
void BlockRectangle::forEachInFrame(Fn fn) const {
    if (_size.width() <= BlockCoordinate{0} || _size.height() <= BlockCoordinate{0}) {
        return;
    }
    if constexpr (std::is_invocable_r_v<void, Fn, BlockPosition, int>) {
        int index = 0;
        for (auto x = BlockCoordinate{0}; x < _size.width(); ++x) {
            fn(BlockPosition{x1() + x, y1()}, index++);
        }
        if (_size.height() > BlockCoordinate{1}) {
            for (auto y = BlockCoordinate{1}; y < _size.height(); ++y) {
                fn(BlockPosition{x2() - 1, y1() + y}, index++);
            }
            for (auto x = _size.width() - BlockCoordinate{2}; x >= BlockCoordinate{0}; --x) {
                fn(BlockPosition{x1() + x, y2() - 1}, index++);
            }
            if (_size.width() > BlockCoordinate{1}) {
                for (auto y = _size.height() - BlockCoordinate{2}; y > BlockCoordinate{0}; --y) {
                    fn(BlockPosition{x1(), y1() + y}, index++);
                }
            }
        }
    } else {
        for (auto x = BlockCoordinate{0}; x < _size.width(); ++x) {
            fn(BlockPosition{x1() + x, y1()});
        }
        if (_size.height() > BlockCoordinate{1}) {
            for (auto y = BlockCoordinate{1}; y < _size.height(); ++y) {
                fn(BlockPosition{x2() - 1, y1() + y});
            }
            for (auto x = _size.width() - BlockCoordinate{2}; x >= BlockCoordinate{0}; --x) {
                fn(BlockPosition{x1() + x, y2() - 1});
            }
            if (_size.width() > BlockCoordinate{1}) {
                for (auto y = _size.height() - BlockCoordinate{2}; y > BlockCoordinate{0}; --y) {
                    fn(BlockPosition{x1(), y1() + y});
                }
            }
        }
    }
}

}
