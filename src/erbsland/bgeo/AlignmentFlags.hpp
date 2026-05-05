// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/EnumFlags.hpp"

#include <cstdint>

namespace erbsland::bgeo {

/// Low-level flags for alignments.
/// @seedoc{/reference/bgeo/alignment_and_orientation}
/// @tested{AlignmentTest}
enum class AlignmentFlag : uint8_t {
    None = 0,
    Left = 1U << 0,    ///< Aligned to the left edge.
    HCenter = 1U << 1, ///< Aligned to the horizontal center.
    Right = 1U << 2,   ///< Aligned to the right edge.
    Top = 1U << 4,     ///< Aligned to the top edge.
    VCenter = 1U << 5, ///< Aligned to the vertical center.
    Bottom = 1U << 6,  ///< Aligned to the bottom edge.
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
    All = Left | HCenter | Right | Top | VCenter | Bottom,
};

/// Low-level alignment flags.
using AlignmentFlags = util::EnumFlags<AlignmentFlag>;

}

template <>
struct std::hash<erbsland::bgeo::AlignmentFlags> {
    auto operator()(const erbsland::bgeo::AlignmentFlags &alignmentFlags) const noexcept -> std::size_t {
        return static_cast<std::size_t>(alignmentFlags.toRawValue());
    }
};
