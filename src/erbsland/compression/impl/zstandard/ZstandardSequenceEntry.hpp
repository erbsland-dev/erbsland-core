// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::compression::impl {

/// One sequence-table transition including its decoded value baseline.
/// @tested{ZstandardInternalTest}
struct ZstandardSequenceEntry final {
    uint32_t baseline{};  ///< Minimum decoded value.
    uint8_t symbol{};     ///< Encoded sequence symbol.
    uint8_t valueBits{};  ///< Additional bits for the decoded value.
    uint8_t stateBits{};  ///< Additional bits for the next FSE state.
    uint16_t stateBase{}; ///< Base for the next FSE state.
};

}
