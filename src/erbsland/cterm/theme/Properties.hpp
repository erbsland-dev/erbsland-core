// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockRole.hpp"

#include "../BlockAttributes.hpp"
#include "../Color.hpp"
#include "../ColorSequence.hpp"

#include "../../bgeo/BlockMargins.hpp"
#include "../../text/Char.hpp"

#include <array>
#include <optional>
#include <utility>

namespace erbsland::cterm::theme {

/// Authoring values for one theme property sheet.
class Properties final {
public:
    /// Number of block roles stored in a property sheet.
    constexpr static auto cBlockCount = std::size_t{16};
    /// The block code point table.
    using Blocks = std::array<text::Char, cBlockCount>;
    /// The special code-point for inherited blocks that inherit from the parent.
    constexpr static auto cInheritedBlock = text::Char{U'\u200B'}; // U+200B ZERO WIDTH SPACE

public:
    /// Access the optional color replacement.
    [[nodiscard]] auto color() const noexcept -> const std::optional<Color> & { return _color; }
    /// Replace the optional color replacement.
    void setColor(Color color) noexcept { _color = color; }
    /// Access the optional color sequence.
    [[nodiscard]] auto colorSequence() const noexcept -> const std::optional<ColorSequence> & { return _colorSequence; }
    /// Replace the optional color sequence.
    void setColorSequence(ColorSequence colorSequence) noexcept { _colorSequence = std::move(colorSequence); }
    /// Access the optional attributes replacement.
    [[nodiscard]] auto attributes() const noexcept -> const std::optional<BlockAttributes> & { return _attributes; }
    /// Replace the optional attributes replacement.
    void setAttributes(BlockAttributes attributes) noexcept { _attributes = attributes; }
    /// Access the optional block table.
    [[nodiscard]] auto blocks() const noexcept -> const std::optional<Blocks> & { return _blocks; }
    /// Replace one block role.
    void setBlock(BlockRole role, text::Char codePoint) noexcept;
    /// Replace the complete block table.
    void setBlocks(Blocks blocks) noexcept { _blocks = blocks; }
    /// Access the optional margins.
    /// bgeo::BlockMargins are always zero or positive.
    [[nodiscard]] auto margins() const noexcept -> const std::optional<bgeo::BlockMargins> & { return _margins; }
    /// Replace the optional margins.
    /// bgeo::BlockMargins are outside a themed part and stay owned by the parent surface.
    void setMargins(const bgeo::BlockMargins margins) noexcept { _margins = margins.expandedPositive(); }
    /// Access the optional padding.
    /// Padding is always zero or positive.
    [[nodiscard]] auto padding() const noexcept -> const std::optional<bgeo::BlockMargins> & { return _padding; }
    /// Replace the optional padding.
    /// Padding is inside a themed part and is painted by text-focused UI elements.
    void setPadding(const bgeo::BlockMargins padding) noexcept { _padding = padding.expandedPositive(); }

private:
    std::optional<Color> _color;                 ///< The color replacement.
    std::optional<ColorSequence> _colorSequence; ///< The color animation sequence.
    std::optional<BlockAttributes> _attributes;  ///< The attributes replacement.
    std::optional<Blocks> _blocks;               ///< The block code points.
    std::optional<bgeo::BlockMargins> _margins;  ///< Parent-owned margins outside the part.
    std::optional<bgeo::BlockMargins> _padding;  ///< Part-owned padding inside the part.
};

}
