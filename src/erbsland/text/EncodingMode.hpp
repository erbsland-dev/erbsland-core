// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::text {

/// Select how a text conversion handles malformed encoding.
enum class EncodingMode : uint8_t {
    Tolerant = 0, ///< Replace invalid input with the Unicode replacement character.
    Strict,       ///< Throw a text encoding exception when invalid input is encountered.
};

}
