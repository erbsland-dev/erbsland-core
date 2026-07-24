// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::text {

/// The case to use for generated ASCII letters.
enum class LetterCase : uint8_t {
    Lowercase = 0, ///< Use lowercase ASCII letters.
    Uppercase = 1, ///< Use uppercase ASCII letters.
};

}
