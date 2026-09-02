// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::geometry {

/// A physical geometry axis.
enum class Axis : uint8_t {
    X = 0, ///< The x-axis.
    Y = 1, ///< The y-axis.
    Z = 2, ///< The z-axis.
};

}
