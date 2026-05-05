// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Block.hpp"

#include "../text/String.hpp"
#include "../text/StringMap.hpp"
#include "../text/StringView.hpp"
#include "../text/u32/U32String.hpp"
#include "../text/u32/U32StringView.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace erbsland::cterm {

class BlockCombinationStyle;
/// Shared pointer for BlockCombinationStyle
using BlockCombinationStylePtr = std::shared_ptr<BlockCombinationStyle>;

/// A style how two characters are visually combined to a new one.
class BlockCombinationStyle {
public:
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

/// A class to use a simple map to combine styles.
/// It uses a map of [current, overlay] -> combined and uses `Color::overlayWith` to combine the colors.
/// If a character is missing in the map, the overlay overwrites the current one.
class SimpleBlockCombinationStyle : public BlockCombinationStyle {
public:
    /// The map, where the key is `<current>+<overlay>`, and the value is the combined character.
    using Map = text::StringMap<text::String>;

public:
    /// Create an empty combination style.
    SimpleBlockCombinationStyle() = default;
    /// Create a new instance from the given map.
    explicit SimpleBlockCombinationStyle(Map map) noexcept;

public: // implement BlockCombinationStyle
    [[nodiscard]] auto combine(const Block &current, const Block &overlay) const noexcept -> Block override;

public:
    /// Access the current map.
    [[nodiscard]] auto map() const noexcept -> const Map &;
    /// Replace the map.
    void setMap(Map map) noexcept;
    /// Add a new entry to the map.
    void add(
        const text::StringView &current, const text::StringView &overlay, const text::StringView &combined) noexcept;

private:
    Map _map;
};

/// A class that combines characters through an indexed result matrix.
/// It uses supported Unicode code points and a compact byte matrix of result indexes.
/// Unknown characters fall back to the overlay character.
class MatrixBlockCombinationStyle : public BlockCombinationStyle {
public:
    /// Create a new matrix-based combination style.
    /// @param characters The supported Unicode characters in matrix index order.
    /// @param resultMatrix The result matrix in row-major order using result indexes.
    /// The matrix size must be `characters.size() * characters.size()`.
    /// @throws std::invalid_argument If the matrix size is invalid or the character count exceeds 255.
    MatrixBlockCombinationStyle(const text::U32StringView &characters, std::span<const uint8_t> resultMatrix);

public: // implement BlockCombinationStyle
    [[nodiscard]] auto combine(const Block &current, const Block &overlay) const noexcept -> Block override;

private:
    /// Resolve one code point to its matrix index.
    /// @param codePoint The Unicode code point to resolve.
    /// @return The matrix index, or 255 if the code point is unsupported.
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
