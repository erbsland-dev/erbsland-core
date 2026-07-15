// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::text {

/// How to handle encoding or Unicode normalization errors.
enum class EncodingErrorMode : uint8_t {
    Throw = 0, ///< Throw a text encoding exception when invalid input is encountered.
    Ignore,    ///< Skip unsupported input when the calling API allows lossy recovery.
    Replace,   ///< Replace unsupported input with the Unicode replacement character when supported.
};

}
