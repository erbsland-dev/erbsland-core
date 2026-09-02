// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::geometry {

/// Symmetry transforms for positions in a block size or rectangle.
enum class Symmetry : uint8_t {
    Identity = 0,       ///< Keep the position unchanged.
    Rotate90,           ///< Rotate 90 degrees counter-clockwise.
    Rotate180,          ///< Rotate 180 degrees.
    Rotate270,          ///< Rotate 270 degrees counter-clockwise.
    MirrorHorizontal,   ///< Mirror horizontally, exchanging left and right.
    MirrorVertical,     ///< Mirror vertically, exchanging top and bottom.
    MirrorDiagonal,     ///< Mirror along the top-left to bottom-right diagonal.
    MirrorAntiDiagonal, ///< Mirror along the top-right to bottom-left diagonal.
};

}
