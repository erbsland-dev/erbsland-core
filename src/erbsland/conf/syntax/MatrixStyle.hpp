// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::conf::syntax {

/// Formatting choice for MatrixStyle.
enum class MatrixStyle : uint8_t {
    Keep,  ///< Preserve row spacing.
    Align, ///< Align value columns.
};

}
