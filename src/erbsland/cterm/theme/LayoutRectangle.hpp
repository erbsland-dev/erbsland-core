// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../bgeo/BlockRectangle.hpp"

namespace erbsland::cterm::theme {

/// bgeo::BlockRectangle set for a themed layout with margins and padding.
struct LayoutRectangles final {
    bgeo::BlockRectangle outerRect;   ///< The complete assigned rectangle, including margins.
    bgeo::BlockRectangle partRect;    ///< The rectangle owned and painted by the themed part.
    bgeo::BlockRectangle contentRect; ///< The rectangle inside the themed part padding.
};

}
