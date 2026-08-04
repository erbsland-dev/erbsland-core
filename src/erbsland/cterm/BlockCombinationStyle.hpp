// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Block.hpp"
#include "BlockCombinationStyle_fwd.hpp"

#include <array>
#include <memory>

namespace erbsland::cterm {

/// A style how two characters are visually combined to a new one.
class BlockCombinationStyle {
public:
    // defaults
    virtual ~BlockCombinationStyle() = default;

public:
    /// Does this style take the surrounding character into account?
    /// If this method returns false, `combine` with current as a single character is called.
    /// If this method returns true, `combine` with current and overlay is called.
    [[nodiscard]] virtual auto isSurroundingAware() const noexcept -> bool { return false; }

    /// Combines the current char with a new that is placed on top of the current one.
    /// The default implementation just returns the overlay character.
    /// @param current The current (lower) character.
    /// @param overlay The new (upper) character that overlays the current one.
    /// @return The combined character.
    [[nodiscard]] virtual auto combine(const Block &current, const Block &overlay) const noexcept -> Block;

    /// Combines the current character situation with a new overlay character that is placed on top of the current one.
    /// The default implementation just returns the overlay character.
    /// The matrix starts at (-1, -1) and ends at (1, 1) (left to right, top to bottom).
    /// @param current A 3x3 matrix with nullptr for locations outside the buffer.
    /// @param overlay The new (upper) character that overlays the current one.
    /// @return The combined character.
    [[nodiscard]] virtual auto combine(const std::array<const Block *, 9> &current, const Block &overlay) const noexcept
        -> Block;

public: // predefined styles
    /// Overwrite the character and color.
    [[nodiscard]] static auto overwrite() noexcept -> const BlockCombinationStylePtr &;
    /// Overwrite the character but overlay the color.
    [[nodiscard]] static auto colorOverlay() noexcept -> const BlockCombinationStylePtr &;
    /// Combine light, double, and heavy frames.
    /// In that order: double overwrites light, and heavy overwrites double and light.
    /// Colors are overlay.
    [[nodiscard]] static auto commonBoxFrame() noexcept -> const BlockCombinationStylePtr &;
};

}
