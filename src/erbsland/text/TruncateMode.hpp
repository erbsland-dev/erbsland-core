// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::text {

/// The side of a string where text is removed when truncating.
enum class TruncateMode : uint8_t {
    End = 0,    ///< Keep the beginning and cut at the end.
    Middle = 1, ///< Keep beginning and end, cutting in the middle.
    Begin = 2,  ///< Keep the end and cut at the beginning.
};

}
