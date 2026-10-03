// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::conf::syntax {

/// Formatting choice for PositionMode.
enum class PositionMode : uint8_t {
    Spaces, ///< A fixed number of separating spaces.
    Column, ///< A zero-based target code-point column.
};

}
