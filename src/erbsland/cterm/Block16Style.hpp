// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Block.hpp"
#include "BlockString.hpp"
#include "FrameStyle.hpp"

#include "../text/String.hpp"
#include "../text/u32/U32String.hpp"

#include <array>
#include <memory>

namespace erbsland::cterm {

class Block16Style;
/// Shared pointer for Block16Style
using Block16StylePtr = std::shared_ptr<Block16Style>;

/// Defines a style for drawing tiles.
class Block16Style {
public:
    /// Create a new tile 16 style.
    /// Connection points/bits: E:0, S:1, W:2, N:3
    explicit Block16Style(std::array<Block, 16> tiles) noexcept : _tiles(std::move(tiles)) {}
    /// Create a new tile 16 style from 16 terminal characters.
    /// Connection points/bits: E:0, S:1, W:2, N:3
    /// @param tiles A sequence of exactly 16 terminal characters.
    /// @throws err::ParameterError If `tiles` does not contain exactly 16 terminal characters.
    explicit Block16Style(const text::String &tiles);
    /// Create a new tile 16 style from 16 terminal characters.
    /// Connection points/bits: E:0, S:1, W:2, N:3
    /// @param tiles A sequence of exactly 16 terminal characters.
    /// @throws err::ParameterError If `tiles` does not contain exactly 16 terminal characters.
    explicit Block16Style(const text::U32String &tiles);

public: // accessors
    /// Access the block for a given bit combination.
    /// @param bitMask The bit-mask E:0, S:1, W:2, N:3.
    /// @return The block for the given bit combination, or an empty block if the bits are out of bounds.
    [[nodiscard]] auto block(uint32_t bitMask) const noexcept -> Block;

public:
    /// Create a new shared style from 16 terminal characters.
    /// @param tiles A sequence of exactly 16 terminal characters.
    /// @return A shared style instance.
    /// @throws err::ParameterError If `tiles` does not contain exactly 16 terminal characters.
    [[nodiscard]] static auto create(const text::String &tiles) -> Block16StylePtr;
    /// Create a new shared style from 16 terminal characters.
    /// @param tiles A sequence of exactly 16 terminal characters.
    /// @return A shared style instance.
    /// @throws err::ParameterError If `tiles` does not contain exactly 16 terminal characters.
    [[nodiscard]] static auto create(const text::U32String &tiles) -> Block16StylePtr;
    /// For drawing light frames.
    [[nodiscard]] static auto lightFrame() -> Block16StylePtr;
    /// For drawing light frames with double-dashed lines.
    [[nodiscard]] static auto lightDoubleDashFrame() -> Block16StylePtr;
    /// For drawing light frames with triple-dashed lines.
    [[nodiscard]] static auto lightTripleDashFrame() -> Block16StylePtr;
    /// For drawing light frames with quadruple-dashed lines.
    [[nodiscard]] static auto lightQuadrupleDashFrame() -> Block16StylePtr;
    /// For drawing light frames with rounded corners.
    [[nodiscard]] static auto lightRoundedFrame() -> Block16StylePtr;
    /// For drawing heavy frames.
    [[nodiscard]] static auto heavyFrame() -> Block16StylePtr;
    /// For drawing heavy frames with double-dashed lines.
    [[nodiscard]] static auto heavyDoubleDashFrame() -> Block16StylePtr;
    /// For drawing heavy frames with triple-dashed lines.
    [[nodiscard]] static auto heavyTripleDashFrame() -> Block16StylePtr;
    /// For drawing heavy frames with quadruple-dashed lines.
    [[nodiscard]] static auto heavyQuadrupleDashFrame() -> Block16StylePtr;
    /// For drawing double frames.
    [[nodiscard]] static auto doubleFrame() -> Block16StylePtr;
    /// For drawing solid block frames.
    [[nodiscard]] static auto fullBlockFrame() -> Block16StylePtr;
    /// For drawing solid block frames with chamfered corners.
    [[nodiscard]] static auto fullBlockWithChamferFrame() -> Block16StylePtr;
    /// For drawing empty frames with colored spaces.
    [[nodiscard]] static auto noneFrame() -> Block16StylePtr;
    /// Get the style for the given frame style.
    /// @param frameStyle The frame style to resolve.
    /// @return The shared style instance, or `nullptr` if `frameStyle` requires `Tile9Style`.
    [[nodiscard]] static auto forStyle(FrameStyle frameStyle) -> Block16StylePtr;

private:
    [[nodiscard]] static auto toTiles(const BlockString &tiles) -> std::array<Block, 16>;

private:
    std::array<Block, 16> _tiles;
};

}
