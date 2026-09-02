// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::block {

template <typename Fn>
void Size::forEach(Fn fn) const {
    for (auto y = Coordinate{0}; y < height(); ++y) {
        for (auto x = Coordinate{0}; x < width(); ++x) {
            fn(Position{x, y});
        }
    }
}

}
