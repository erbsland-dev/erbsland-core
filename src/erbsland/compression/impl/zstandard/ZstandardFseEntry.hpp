// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::compression::impl {

/// One state transition in a finite-state entropy decoding table.
/// @tested{ZstandardInternalTest}
struct ZstandardFseEntry final {
    uint8_t symbol{};     ///< Symbol emitted for this state.
    uint8_t bitCount{};   ///< Bits consumed to select the next state.
    uint16_t stateBase{}; ///< Base added to those bits.
};

}
