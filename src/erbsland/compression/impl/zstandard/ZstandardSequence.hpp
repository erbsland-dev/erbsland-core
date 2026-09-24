// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::compression::impl {

/// One encoded Zstandard literal-and-match sequence.
/// @tested{ZstandardInternalTest}
struct ZstandardSequence final {
    uint32_t literalLength{}; ///< Literals preceding the match.
    uint32_t offset{};        ///< Backward match offset.
    uint32_t matchLength{};   ///< Match length.
    uint8_t literalSymbol{};  ///< Encoded literal-length symbol.
    uint8_t literalBits{};    ///< Literal-length extra-bit count.
    uint32_t literalExtra{};  ///< Literal-length extra value.
    uint8_t offsetSymbol{};   ///< Encoded offset symbol.
    uint8_t offsetBits{};     ///< Offset extra-bit count.
    uint32_t offsetExtra{};   ///< Offset extra value.
    uint8_t matchSymbol{};    ///< Encoded match-length symbol.
    uint8_t matchBits{};      ///< Match-length extra-bit count.
    uint32_t matchExtra{};    ///< Match-length extra value.
};

}
