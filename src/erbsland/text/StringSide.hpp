// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::text {

/// Select one side of a string-like value.
enum class StringSide : uint8_t {
    Front, ///< The start of the string.
    Back,  ///< The end of the string.
};

}
