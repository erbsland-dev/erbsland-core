// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockCombinationStyle.hpp"

#include "../text/u32/U32String.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace erbsland::cterm {

/// Combine block characters through an indexed result matrix.
/// @tested{BlockCombinationStyleTest}
class MatrixBlockCombinationStyle : public BlockCombinationStyle {
public:
    /// Create a new matrix-based combination style.
    /// @param characters The supported Unicode characters in matrix index order.
    /// @param resultMatrix The result matrix in row-major order using result indexes.
    /// @throws err::ParameterError If the matrix size is invalid or the character count exceeds 255.
    MatrixBlockCombinationStyle(const text::U32String &characters, std::span<const uint8_t> resultMatrix);

public: // implement BlockCombinationStyle
    [[nodiscard]] auto combine(const Block &current, const Block &overlay) const noexcept -> Block override;

private:
    /// Resolve one code point to its matrix index.
    [[nodiscard]] auto lookupIndex(text::Char codePoint) const noexcept -> uint8_t;

private:
    static constexpr auto cUnsupportedIndex = uint8_t{0xFFU};

private:
    text::U32String _characters;
    std::vector<uint8_t> _resultMatrix;
    text::Char _lookupBase{};
    std::vector<uint8_t> _characterIndexByCodePoint;
};

}
