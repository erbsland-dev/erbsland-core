// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AlignedSource_fwd.hpp"
#include "Rectangle.hpp"

namespace erbsland::block {

/// Effective source and target rectangles after alignment.
/// @seedoc{/reference/block/block_geometry}
/// @tested{RectangleTest}
struct AlignedSource final {
    Rectangle targetRect; ///< The target rectangle inside the alignment box.
    Rectangle sourceRect; ///< The source rectangle after alignment-based cropping.

    auto operator==(const AlignedSource &other) const noexcept -> bool = default;
};

}
