// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::path {

/// Select which component boundary starts a path-glob match.
enum class PathGlobMatchMode : std::uint8_t {
    Full,   ///< Consume every non-root component.
    Suffix, ///< Start at any component boundary, unless the pattern is anchored.
};

}
