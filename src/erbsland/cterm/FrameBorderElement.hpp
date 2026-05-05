// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cterm {

/// Named line groups for a grid frame border.
enum class FrameBorderElement : uint8_t {
    Top,    ///< The top outer line.
    Bottom, ///< The bottom outer line.
    Left,   ///< The left outer line.
    Right,  ///< The right outer line.
    HLine,  ///< The horizontal separator lines between rows.
    VLine,  ///< The vertical separator lines between columns.
};

}
