// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LzmaLengthProbabilities.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace erbsland::compression::impl {

/// Owns the adaptive probability models for an LZMA stream.
/// @tested{ByteCompressionTest}
class LzmaProbabilities final {
public:
    static constexpr auto cPositionStates = std::size_t{16U};  ///< Number of LZMA position states.
    static constexpr auto cLiteralContexts = std::size_t{16U}; ///< Supported literal probability contexts.

public:
    /// Initialize every probability model to its neutral state.
    LzmaProbabilities();
    /// Initialize a length probability model.
    static void initializeLzmaLengthProbabilities(LzmaLengthProbabilities &length);

public:
    std::array<uint16_t, 12U * cPositionStates> isMatch{};
    std::array<uint16_t, 12U> isRep{};
    std::array<uint16_t, 12U> isRepG0{};
    std::array<uint16_t, 12U> isRepG1{};
    std::array<uint16_t, 12U> isRepG2{};
    std::array<uint16_t, 12U * cPositionStates> isRep0Long{};
    std::array<uint16_t, cLiteralContexts * 0x300U> literals{};
    LzmaLengthProbabilities matchLzmaLengthProbabilities;
    LzmaLengthProbabilities repLzmaLengthProbabilities;
    std::array<uint16_t, 4U * 64U> posSlot{};
    std::array<uint16_t, 114U> posDecoders{};
    std::array<uint16_t, 16U> posAlign{};
};

}
