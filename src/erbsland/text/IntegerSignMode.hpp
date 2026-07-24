// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::text {

/// Sign handling for integer text formatting.
enum class IntegerSignMode : uint8_t {
    NegativeOnly = 0, ///< Emit a sign only for negative values.
    Always = 1,       ///< Emit `+` for positive values and `-` for negative values.
    Space = 2,        ///< Emit a space for positive values and `-` for negative values.
};

}
