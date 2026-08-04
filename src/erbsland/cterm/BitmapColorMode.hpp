// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cterm {

/// The mode how color is applied to the bitmap.
enum class BitmapColorMode : uint8_t {
    /// Uses one color from the sequence for the whole bitmap.
    /// The selected sequence entry is `animationCycle + colorAnimationOffset`.
    OneColor,
    /// Uses the color sequence in vertical stripes.
    /// The selected sequence entry is `x + animationCycle + colorAnimationOffset`.
    VerticalStripes,
    /// Uses the color sequence in horizontal stripes.
    /// The selected sequence entry is `y + animationCycle + colorAnimationOffset`.
    HorizontalStripes,
    /// Uses the color sequence in forward diagonal stripes.
    /// The selected sequence entry is `x + y + animationCycle + colorAnimationOffset`.
    ForwardDiagonalStripes,
    /// Uses the color sequence in backward diagonal stripes.
    /// The selected sequence entry is `-x + y + animationCycle + colorAnimationOffset`.
    BackwardDiagonalStripes,
};

}
