// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::geometry {

/// The number of spatial dimensions represented by a geometry value.
enum class Dimensionality : uint8_t {
    One = 1,   ///< One-dimensional geometry.
    Two = 2,   ///< Two-dimensional geometry.
    Three = 3, ///< Three-dimensional geometry.
};

}
