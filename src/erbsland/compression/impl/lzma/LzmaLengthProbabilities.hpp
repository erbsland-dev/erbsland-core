// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <array>
#include <cstdint>

namespace erbsland::compression::impl {

/// Adaptive probability model for LZMA match lengths.
/// @tested{ByteCompressionTest}
struct LzmaLengthProbabilities final {
    static constexpr auto cInitialProbability = uint16_t{1024U};       ///< Neutral binary probability.
    static constexpr auto cPositionStateCount = std::size_t{16U};      ///< Number of position states.
    static constexpr auto cLowSymbolCount = std::size_t{8U};           ///< Low length symbols per position state.
    static constexpr auto cMidSymbolCount = std::size_t{8U};           ///< Mid length symbols per position state.
    static constexpr auto cHighSymbolCount = std::size_t{256U};        ///< High length symbols.

    uint16_t choice{cInitialProbability};                              ///< Selects the low length tree.
    uint16_t choice2{cInitialProbability};                             ///< Selects the mid or high length tree.
    std::array<uint16_t, cPositionStateCount * cLowSymbolCount> low{}; ///< Low length probability trees.
    std::array<uint16_t, cPositionStateCount * cMidSymbolCount> mid{}; ///< Mid length probability trees.
    std::array<uint16_t, cHighSymbolCount> high{};                     ///< High length probability tree.
};

}
