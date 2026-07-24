// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::text {

/// The supported runtime format argument kind.
enum class FormatArgumentKind : uint8_t {
    None = 0,
    U8Text = 1,
    U16Text = 2,
    U32Text = 3,
    SignedInteger = 4,
    UnsignedInteger = 5,
    FloatingPoint = 6,
    Boolean = 7,
    Character = 8,
    Bytes = 9,
};

}
