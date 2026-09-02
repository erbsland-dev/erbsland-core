// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/EnumFlags.hpp"

#include <cstddef>
#include <cstdint>

namespace erbsland::geometry {

/// Low-level flags that select a position in a two-dimensional area.
/// @seedoc{/reference/geometry/geometry}
enum class AnchorFlag : uint8_t {
    None = 0,                                              ///< Do not select an axis component.
    Top = 1U << 0,                                         ///< Select the top edge.
    VCenter = 1U << 1,                                     ///< Select the vertical center.
    Bottom = 1U << 2,                                      ///< Select the bottom edge.
    Left = 1U << 4,                                        ///< Select the left edge.
    HCenter = 1U << 5,                                     ///< Select the horizontal center.
    Right = 1U << 6,                                       ///< Select the right edge.
    TopLeft = Top | Left,                                  ///< Select the top-left corner.
    TopCenter = Top | HCenter,                             ///< Select the top-center edge.
    TopRight = Top | Right,                                ///< Select the top-right corner.
    CenterLeft = VCenter | Left,                           ///< Select the center-left edge.
    Center = VCenter | HCenter,                            ///< Select the center.
    CenterRight = VCenter | Right,                         ///< Select the center-right edge.
    BottomLeft = Bottom | Left,                            ///< Select the bottom-left corner.
    BottomCenter = Bottom | HCenter,                       ///< Select the bottom-center edge.
    BottomRight = Bottom | Right,                          ///< Select the bottom-right corner.
    HorizontalMask = Left | HCenter | Right,               ///< Select all horizontal flags.
    VerticalMask = Top | VCenter | Bottom,                 ///< Select all vertical flags.
    All = Top | VCenter | Bottom | Left | HCenter | Right, ///< Select every anchor flag.
};

/// A set of low-level anchor flags.
using AnchorFlags = util::EnumFlags<AnchorFlag>;

}

template <>
struct std::hash<erbsland::geometry::AnchorFlags> {
    auto operator()(const erbsland::geometry::AnchorFlags &anchorFlags) const noexcept -> std::size_t {
        return static_cast<std::size_t>(anchorFlags.toRawValue());
    }
};
