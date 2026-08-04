// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cterm {

/// The mode how the bitmap is scaled.
enum class BitmapScaleMode : uint8_t {
    /// Draw the bitmap with half-blocks.
    /// This mode uses the 16 characters from `halfBlocks()`.
    /// Each 2x2 pixel block creates a 4-bit index in this order:
    /// bit 0 = top-left, bit 1 = top-right, bit 2 = bottom-left, bit 3 = bottom-right.
    /// This renders the bitmap at half width and half height, rounded up.
    /// Character colors are overlaid on the color from the color mode.
    HalfBlock,
    /// Draw the bitmap with full-blocks.
    /// This mode uses `fullBlock()` to draw each set pixel of the bitmap.
    /// Character colors are overlaid on the color from the color mode.
    /// To color the unset pixels, you must fill the bitmap area first.
    FullBlock,
    /// Draw the bitmap with double-blocks.
    /// This mode uses `doubleBlocks()` to draw each set pixel of the bitmap.
    /// The bitmap is twice as large in the X axis, to compensate for the rectangular shape of terminal characters.
    /// Character colors are overlaid on the color from the color mode.
    /// To color the unset pixels, you must fill the bitmap area first.
    DoubleBlock,
};

}
