// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cterm {

/// A modifier pressed together with a key.
enum class KeyModifier : uint8_t {
    /// The Shift key.
    Shift = 1 << 0,
    /// The Control key.
    Control = 1 << 1,
    /// The Alt key.
    Alt = 1 << 2,
};

}
