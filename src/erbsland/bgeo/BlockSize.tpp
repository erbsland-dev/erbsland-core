// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::bgeo {

template <typename Fn>
void BlockSize::forEach(Fn fn) const {
    for (auto y = BlockCoordinate{0}; y < height(); ++y) {
        for (auto x = BlockCoordinate{0}; x < width(); ++x) {
            fn(BlockPosition{x, y});
        }
    }
}

}
