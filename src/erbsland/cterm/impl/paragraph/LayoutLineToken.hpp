// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../BlockCount.hpp"
#include "../../BlockIndex.hpp"
#include "../../BlockString_fwd.hpp"

#include <cassert>
#include <cstdint>
#include <optional>

namespace erbsland::cterm::impl::paragraph {

/// One prepared source-line token used during physical line layout.
class LayoutLineToken final {
public:
    /// The kind of token.
    enum class Type : uint8_t {
        Word,            ///< A contiguous source word.
        IndivisibleWord, ///< A source range that must be emitted as one unit.
        SeparatorSpace,  ///< A collapsed separator run rendered as one space.
        Tab,             ///< A left-aligned tab resolved later from the current column.
    };

    /// The result of splitting a source word for one physical line.
    struct SplitResult final {
        BlockCount sourceCharacterCount{}; ///< The number of source characters written to the current line.
        int sourceWidth = 0;               ///< The display width of the written source characters.
        int width = 0;                     ///< The total rendered width including any split marker.
    };

public:
    /// Create one prepared source-line token.
    /// @param type The token type.
    /// @param startIndex The first source character index covered by the token.
    /// @param length The number of source characters covered by the token.
    /// @param displayWidth The cached display width of the token.
    constexpr LayoutLineToken(
        const Type type, const BlockIndex startIndex, const BlockCount length, const int displayWidth) noexcept :
        _type{type}, _startIndex{startIndex}, _length{length}, _displayWidth{displayWidth} {}

    // defaults
    LayoutLineToken(const LayoutLineToken &) = default;
    LayoutLineToken(LayoutLineToken &&) noexcept = default;
    auto operator=(const LayoutLineToken &) -> LayoutLineToken & = default;
    auto operator=(LayoutLineToken &&) noexcept -> LayoutLineToken & = default;

public:
    /// Get the token type.
    [[nodiscard]] auto type() const noexcept -> Type { return _type; }
    /// Get the first source character index of the token.
    [[nodiscard]] auto startIndex() const noexcept -> BlockIndex { return _startIndex; }
    /// Get the source character count of the token.
    [[nodiscard]] auto length() const noexcept -> BlockCount { return _length; }
    /// Get the cached display width of the token.
    [[nodiscard]] auto displayWidth() const noexcept -> int { return _displayWidth; }
    /// Test whether this word token is indivisible.
    /// @return `true` if the token must never be split.
    [[nodiscard]] auto isIndivisible() const noexcept -> bool { return _type == Type::IndivisibleWord; }
    /// Get the source character index for the given offset inside the word.
    [[nodiscard]] auto sourceIndex(BlockCount offset) const noexcept -> BlockIndex {
        assert(isWord());
        return _startIndex + offset;
    }
    /// Get the remaining display width from the given character offset onward.
    [[nodiscard]] auto remainingWidth(const BlockString &text, BlockCount offset) const noexcept -> int;
    /// Split the word to fit into the available width.
    /// @param text The source text that contains the word characters.
    /// @param offset The first source character offset to place on the current line.
    /// @param availableWidth The available display width for the word content and optional trailing marker.
    /// @param trailingMarkerWidth The width of an optional trailing split marker.
    /// @return The split result, or `std::nullopt` if nothing can be rendered.
    [[nodiscard]] auto split(
        const BlockString &text, BlockCount offset, int availableWidth, int trailingMarkerWidth) const noexcept
        -> std::optional<SplitResult>;

private:
    /// Test whether this token represents either kind of word.
    [[nodiscard]] auto isWord() const noexcept -> bool { return _type == Type::Word || _type == Type::IndivisibleWord; }

private:
    Type _type = Type::Word;
    BlockIndex _startIndex{};
    BlockCount _length{};
    int _displayWidth = 0;
};

}
