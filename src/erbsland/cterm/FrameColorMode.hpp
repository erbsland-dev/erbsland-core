// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cterm {

/// The mode how animated colors are applied to frames.
enum class FrameColorMode : uint8_t {
    /// Uses one color from the sequence for the whole frame or fill area.
    /// The selected sequence entry is `animationCycle + animationOffset`.
    OneColor,
    /// Uses the color sequence in vertical stripes.
    /// The selected sequence entry is `x + animationCycle + animationOffset`.
    VerticalStripes,
    /// Uses the color sequence in horizontal stripes.
    /// The selected sequence entry is `y + animationCycle + animationOffset`.
    HorizontalStripes,
    /// Uses the color sequence in forward diagonal stripes.
    /// The selected sequence entry is `x + y + animationCycle + animationOffset`.
    ForwardDiagonalStripes,
    /// Uses the color sequence in backward diagonal stripes.
    /// The selected sequence entry is `-x + y + animationCycle + animationOffset`.
    BackwardDiagonalStripes,
    /// Uses the color sequence along the frame border in clockwise order.
    /// Increasing `animationCycle` makes the colors travel clockwise.
    ChasingBorderCW,
    /// Uses the color sequence along the frame border in clockwise order.
    /// Increasing `animationCycle` makes the colors travel counter-clockwise.
    ChasingBorderCCW,
};

}
