// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::text {

/// Result status when reading a number from a string reader.
enum class ReadNumberStatus : uint8_t {
    Success,
    NoDigits,
    TooFewDigits,
    TooManyDigits,
    Overflow,
    ParseError,
};

}
