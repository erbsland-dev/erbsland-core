// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::text {

/// The capitalization to use for generated words.
enum class Capitalization : uint8_t {
    Lowercase = 0, ///< Use all lowercase letters.
    Uppercase = 1, ///< Use all uppercase letters.
    Titlecase = 2, ///< Use an uppercase first letter and lowercase remaining letters.
};

}
