// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::text {

/// Reserved non-character signals stored in `Char` values.
/// @tested{CharTest}
enum class CharSignal : uint8_t {
    EndOfData,
    NoCodePoint,
};

}
