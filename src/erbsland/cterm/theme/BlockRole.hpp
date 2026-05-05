// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cterm::theme {

/// Named roles for the sixteen themed block code points.
enum class BlockRole : uint8_t {
    NorthWest = 0,         ///< Top-left block.
    North = 1,             ///< Top edge block.
    NorthEast = 2,         ///< Top-right block.
    West = 3,              ///< Left edge block.
    Center = 4,            ///< Center block.
    East = 5,              ///< Right edge block.
    SouthWest = 6,         ///< Bottom-left block.
    South = 7,             ///< Bottom edge block.
    SouthEast = 8,         ///< Bottom-right block.
    HorizontalWest = 9,    ///< Left block for one-row rectangles.
    HorizontalCenter = 10, ///< Center block for one-row rectangles.
    HorizontalEast = 11,   ///< Right block for one-row rectangles.
    VerticalNorth = 12,    ///< Top block for one-column rectangles.
    VerticalCenter = 13,   ///< Center block for one-column rectangles.
    VerticalSouth = 14,    ///< Bottom block for one-column rectangles.
    Single = 15,           ///< Single-cell block.

    // The main block, if only one is used
    Main = Center, ///< Alias for the main block, if only one is relevant.

    // Part::Text (for one line texts)
    LeftPadding = West,  ///< Padding on the left side.
    RightPadding = East, ///< Padding on the right side.

    // Part::Bracket - like "[ x / y / z ]"
    LeftBracket = HorizontalWest,  ///< Left brackets.
    MiddleBracket = Center,        ///< Middle "brackets" (separator).
    RightBracket = HorizontalEast, ///< Right brackets.
    LeftOuterPadding = SouthWest,  ///< Left outer padding (after left bracket).
    RightOuterPadding = SouthEast, ///< Right outer padding (before right bracket).
    LeftInnerPadding = NorthWest,  ///< Left inner padding (after middle bracket).
    RightInnerPadding = NorthEast, ///< Right inner padding (before middle bracket).
};

}
