// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::cterm {

/// Supported text animation styles used by `Buffer::renderText()`.
enum class BlockTextAnimation : uint8_t {
    None,          ///< No animation.
    ColorDiagonal, ///< Diagonal color animation. Requires a color sequence.
};

}
