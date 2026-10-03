// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::conf::syntax {

/// Formatting choice for VectorStyle.
enum class VectorStyle : uint8_t {
    Keep,      ///< Preserve row layout.
    Compact,   ///< Use one line when it fits.
    Multiline, ///< Use one value per continued line.
};

}
