// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::system {

/// Select the base environment inherited before applying changes.
enum class EnvironmentInherit : uint8_t {
    None, ///< Start with no inherited variables.
    All,  ///< Inherit every variable from the base environment.
};

}
