// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockRectangle.hpp"

namespace erbsland::bgeo {

/// Effective source and target rectangles after alignment.
/// @seedoc{/reference/bgeo/block_geometry}
/// @tested{BlockRectTest}
struct BlockAlignedSource final {
    BlockRectangle targetRect; ///< The target rectangle inside the alignment box.
    BlockRectangle sourceRect; ///< The source rectangle after alignment-based cropping.

    auto operator==(const BlockAlignedSource &other) const noexcept -> bool = default;
};

}
