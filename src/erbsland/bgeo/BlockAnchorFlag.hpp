// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/EnumFlags.hpp"

#include <cstdint>

namespace erbsland::bgeo {

/// Low-level flags for block anchors.
/// @seedoc{/reference/bgeo/block_geometry}
enum class BlockAnchorFlag : uint8_t {
    None = 0,
    Top = 1U << 0,     ///< Anchored at the top edge.
    VCenter = 1U << 1, ///< Anchored at the vertical center.
    Bottom = 1U << 2,  ///< Anchored at the bottom edge.
    Left = 1U << 4,    ///< Anchored at the left edge.
    HCenter = 1U << 5, ///< Anchored at the horizontal center.
    Right = 1U << 6,   ///< Anchored at the right edge.
    TopLeft = Top | Left,
    TopCenter = Top | HCenter,
    TopRight = Top | Right,
    CenterLeft = VCenter | Left,
    Center = VCenter | HCenter,
    CenterRight = VCenter | Right,
    BottomLeft = Bottom | Left,
    BottomCenter = Bottom | HCenter,
    BottomRight = Bottom | Right,
    HorizontalMask = Left | HCenter | Right,
    VerticalMask = Top | VCenter | Bottom,
    All = Top | VCenter | Bottom | Left | HCenter | Right,
};

/// Low-level block anchor flags.
using BlockAnchorFlags = util::EnumFlags<BlockAnchorFlag>;

}
