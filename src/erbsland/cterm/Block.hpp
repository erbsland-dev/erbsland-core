// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockStyle.hpp"

#include "impl/BlockTextUtil.hpp"
#include "impl/CombinedBlock.hpp"
#include "impl/TypeTraits.hpp"

#include "../text/Char.hpp"
#include "../text/EncodingErrorMode.hpp"
#include "../text/String.hpp"
#include "../text/u32/U32String.hpp"
#include "../text/u32/U32StringView.hpp"

#include <cstddef>
#include <functional>

namespace erbsland::cterm {

/// Represents a character string with combined terminal style information.
/// Used by the UI code to render colored text blocks on the console.
class Block {
public:
    /// Construct an empty block character using inherited colors.
    constexpr Block() noexcept = default;
    /// Construct a block character from a single Unicode code point using inherited colors.
    /// @param character The base Unicode code point.
    constexpr explicit Block(const text::Char character) noexcept : _character{impl::safeCodePoint(character)} {}
    /// Construct a block character with inherited colors.
    /// @param charStr The UTF-8 encoded text to display.
    /// Invalid or unsupported text normalizes deterministically to a single renderable character.
    /// Empty input, control codes, and leading zero-width code points normalize to `U+FFFD`.
    /// Later visible code points also collapse the result to `U+FFFD`, while a third combining mark is ignored.
    explicit Block(const text::StringView &charStr) noexcept : Block{charStr, BlockStyle{}} {}
    /// Construct a block character with inherited colors.
    /// @param charStr The UTF-32 encoded text to display.
    /// Invalid or unsupported text normalizes deterministically to a single renderable character.
    /// Empty input, control codes, invalid Unicode scalar values, and leading zero-width code points normalize to
    /// `U+FFFD`. Later visible code points also collapse the result to `U+FFFD`, while a third combining mark is
    /// ignored.
    explicit Block(const text::U32StringView &charStr) noexcept : Block{charStr, BlockStyle{}} {}
    /// Construct a block character from a single Unicode code point with explicit style.
    /// @param character The base Unicode code point.
    /// @param style The style for the character.
    constexpr Block(const text::Char character, const BlockStyle style) noexcept :
        _character{impl::safeCodePoint(character)}, _style{style} {}
    /// Construct a block character from a single Unicode code point with explicit color and attributes.
    /// @param character The base Unicode code point.
    /// @param color The color for the character.
    /// @param attributes The character attributes.
    constexpr Block(const text::Char character, const Color color, const BlockAttributes attributes) noexcept :
        Block{character, BlockStyle{color, attributes}} {}
    /// Construct a block character with explicit text and style.
    /// @param charStr The UTF-8 encoded text to display.
    /// @param style The style for the character.
    /// Invalid or unsupported text normalizes deterministically to a single renderable character.
    /// Empty input, control codes, and leading zero-width code points normalize to `U+FFFD`.
    /// Later visible code points also collapse the result to `U+FFFD`, while a third combining mark is ignored.
    explicit Block(const text::StringView &charStr, BlockStyle style) noexcept : _character{charStr}, _style{style} {}
    /// Construct a block character with explicit text, color, and attributes.
    /// @param charStr The UTF-8 encoded text to display.
    /// @param color The color for the character.
    /// @param attributes The character attributes.
    /// Invalid or unsupported text normalizes deterministically to a single renderable character.
    /// Empty input, control codes, and leading zero-width code points normalize to `U+FFFD`.
    /// Later visible code points also collapse the result to `U+FFFD`, while a third combining mark is ignored.
    explicit Block(const text::StringView &charStr, const Color color, const BlockAttributes attributes) noexcept :
        Block{charStr, BlockStyle{color, attributes}} {}
    /// Construct a block character with explicit text and style.
    /// @param charStr The UTF-32 encoded text to display.
    /// @param style The style for the character.
    /// Invalid or unsupported text normalizes deterministically to a single renderable character.
    /// Empty input, control codes, invalid Unicode scalar values, and leading zero-width code points normalize to
    /// `U+FFFD`. Later visible code points also collapse the result to `U+FFFD`, while a third combining mark is
    /// ignored.
    explicit Block(const text::U32StringView &charStr, const BlockStyle style) noexcept :
        _character{charStr}, _style{style} {}
    /// Construct a block character with explicit text, color, and attributes.
    /// @param charStr The UTF-32 encoded text to display.
    /// @param color The color for the character.
    /// @param attributes The character attributes.
    /// Invalid or unsupported text normalizes deterministically to a single renderable character.
    /// Empty input, control codes, invalid Unicode scalar values, and leading zero-width code points normalize to
    /// `U+FFFD`. Later visible code points also collapse the result to `U+FFFD`, while a third combining mark is
    /// ignored.
    explicit Block(const text::U32StringView &charStr, const Color color, const BlockAttributes attributes) noexcept :
        Block{charStr, BlockStyle{color, attributes}} {}
    /// Construct a block character from a single Unicode code point and a color.
    /// @param character The base Unicode code point.
    /// @param color The color for the character.
    template <typename... tColorArgs>
        requires CharColorConstructorArgs<tColorArgs...>
    constexpr Block(const text::Char character, tColorArgs... color) noexcept :
        _character{impl::safeCodePoint(character)}, _style{Color{color...}} {}
    /// Construct a block character with explicit text and colors.
    /// @param charStr The UTF-8 encoded text to display.
    /// @param color The color for the character.
    /// Invalid or unsupported text normalizes deterministically to a single renderable character.
    /// Empty input, control codes, and leading zero-width code points normalize to `U+FFFD`.
    /// Later visible code points also collapse the result to `U+FFFD`, while a third combining mark is ignored.
    template <typename... tColorArgs>
        requires CharColorConstructorArgs<tColorArgs...>
    explicit Block(const text::StringView &charStr, tColorArgs... color) noexcept :
        _character{charStr}, _style{Color{color...}} {}
    /// Construct a block character with explicit text and colors.
    /// @param charStr The UTF-32 encoded text to display.
    /// @param color The color for the character.
    /// Invalid or unsupported text normalizes deterministically to a single renderable character.
    /// Empty input, control codes, invalid Unicode scalar values, and leading zero-width code points normalize to
    /// `U+FFFD`. Later visible code points also collapse the result to `U+FFFD`, while a third combining mark is
    /// ignored.
    template <typename... tColorArgs>
        requires CharColorConstructorArgs<tColorArgs...>
    explicit Block(const text::U32StringView &charStr, tColorArgs... color) noexcept :
        _character{charStr}, _style{Color{color...}} {}

    // defaults
    Block(const Block &) = default;
    Block(Block &&) = default;
    auto operator=(const Block &) -> Block & = default;
    auto operator=(Block &&) -> Block & = default;

public: // operators
    /// Compare two terminal characters for equality.
    [[nodiscard]] auto operator==(const Block &other) const noexcept -> bool {
        return _character == other._character && _style == other._style;
    }
    /// Compare two terminal characters for inequality.
    [[nodiscard]] auto operator!=(const Block &other) const noexcept -> bool { return !(*this == other); }
    /// Compare just a single-code point character, without the color.
    [[nodiscard]] auto operator==(const text::Char other) const noexcept -> bool {
        return _character.operator==(other);
    }
    /// Compare just a single-code point character, without the color.
    [[nodiscard]] auto operator!=(const text::Char other) const noexcept -> bool { return !operator==(other); }

public: // accessors
    /// Get the character string as UTF-8 text.
    /// @return A UTF-8 encoded copy of the stored character sequence.
    [[nodiscard]] auto charStr() const -> text::String { return _character.utf8(); }
    /// Get the character string as UTF-32 text.
    /// @return A UTF-32 encoded copy of the stored character sequence.
    [[nodiscard]] auto u32CharStr() const -> text::U32String { return _character.utf32(); }
    /// Get the leading Unicode code point.
    /// @return The base code point, or `0` if this character is empty.
    [[nodiscard]] constexpr auto mainCodePoint() const noexcept -> text::Char { return _character.mainCodePoint(); }
    /// Get a single Unicode code point or zero for combined or empty characters.
    /// This is a fast-path method for comparing a single-code point character, without the color.
    /// @return The single code point, or `0` if this character is combined or empty.
    [[nodiscard]] constexpr auto singleCodePoint() const noexcept -> text::Char { return _character.singleCodePoint(); }
    /// Get the stored Unicode code points.
    /// Unused entries are set to `0`.
    [[nodiscard]] constexpr auto codePoints() const noexcept -> const impl::CombinedBlock::Storage & {
        return _character.codePoints();
    }
    /// Get the number of stored Unicode code points.
    [[nodiscard]] constexpr auto codePointCount() const noexcept -> std::size_t { return _character.codePointCount(); }
    /// Get the character color.
    [[nodiscard]] auto color() const noexcept -> Color { return _style.color(); }
    /// Get the character attributes.
    [[nodiscard]] auto attributes() const noexcept -> BlockAttributes { return _style.attributes(); }
    /// Get the combined character style.
    [[nodiscard]] auto style() const noexcept -> const BlockStyle & { return _style; }
    /// Get the display width on a terminal in cells.
    [[nodiscard]] auto displayWidth() const noexcept -> int { return _character.displayWidth(); }
    /// Get the number of UTF-8 bytes needed to encode this character.
    [[nodiscard]] auto byteCount() const noexcept -> std::size_t { return _character.byteCount(); }

public: // modifiers
    /// Replace the full style of this character.
    /// @param style The new style.
    void setStyle(const BlockStyle style) noexcept { _style = style; }
    /// Create a character with an additional combining code point appended.
    /// @param codePoint The combining code point to append.
    /// @param encodingErrors How invalid combining code points are handled.
    /// @return A copy of this character with the combining code point appended.
    /// `EncodingErrorMode::Replace` and `EncodingErrorMode::Ignore` keep the original character unchanged for invalid
    /// combining code points or when the storage is already full.
    /// @throws std::invalid_argument If `encodingErrors` is `EncodingErrorMode::Throw` and the code point is
    /// unsupported.
    [[nodiscard]] auto withCombining(
        text::Char codePoint, text::EncodingErrorMode encodingErrors = text::EncodingErrorMode::Replace) const -> Block;
    /// Create a character with style applied on top of the stored style.
    /// `Inherited` color components keep the current color component, and unspecified attributes keep the current
    /// attribute state.
    /// @param style The style override to apply.
    /// @return A copy of this character with the overlaid style.
    [[nodiscard]] auto withOverlay(BlockStyle style) const noexcept -> Block;
    /// Create a character with the given color replacing the stored color.
    /// @param color The replacement color.
    /// @return A copy of this character with exactly `color`.
    [[nodiscard]] auto withColorReplaced(Color color) const noexcept -> Block;
    /// Create a character with the given character style replaced.
    /// @param style The replacement style.
    /// @return A copy of this character with exactly `style`.
    [[nodiscard]] auto withStyleReplaced(BlockStyle style) const noexcept -> Block;
    /// Create a character with the given attributes replacing the stored attributes.
    /// @param attributes The replacement attributes.
    /// @return A copy of this character with exactly `attributes`.
    [[nodiscard]] auto withAttributes(BlockAttributes attributes) const noexcept -> Block;
    /// Create a character with style used as the base underneath the stored style.
    /// The stored color and stored attributes overwrite the base style.
    /// @param style The base style.
    /// @return A copy of this character resolved against the base style.
    [[nodiscard]] auto withBase(BlockStyle style) const noexcept -> Block;
    /// Create a character with another character used as the style base.
    /// Only the style is used from `base`; the stored Unicode character is preserved.
    /// @param base The character providing the base color and attributes.
    /// @return A copy of this character resolved against `base`.
    [[nodiscard]] auto withBase(const Block &base) const noexcept -> Block;

public: // tests
    /// Test if this character is empty (has no code-point).
    [[nodiscard]] constexpr auto isEmpty() const noexcept -> bool { return _character.isEmpty(); }
    /// Test if this character is spacing.
    /// Tests for space, tab, newline, and CR.
    [[nodiscard]] auto isSpacing() const noexcept -> bool;
    /// Test if this character is a control character.
    [[nodiscard]] auto isControl() const noexcept -> bool;
    /// Test if this character is one of the given characters.
    /// Tests only the character, not the color. Only tests one code-point characters.
    [[nodiscard]] auto isOneOf(const text::U32StringView &characters) const noexcept -> bool;
    /// @overload
    [[nodiscard]] auto isOneOf(std::initializer_list<text::Char> characters) const noexcept -> bool;
    /// @overload
    template <typename... tCharacters>
        requires(sizeof...(tCharacters) >= 1 && (std::convertible_to<tCharacters, text::Char> && ...))
    [[nodiscard]] auto isOneOf(tCharacters... characters) const noexcept -> bool {
        return ((*this == text::Char{characters}) || ...);
    }
    /// Compare how two characters would appear on screen.
    /// Code points must match exactly. When `colorEnabled` is `true`, inherited color components are treated as the
    /// terminal default color before comparing. When `attributeEnabled` is `true`, inherited attributes are treated as
    /// disabled before comparing.
    /// @param other The character to compare with.
    /// @param colorEnabled `true` to include colors in the comparison.
    /// @param attributeEnabled `true` to include character attributes in the comparison.
    /// @return `true` if both characters render identically.
    [[nodiscard]] auto renderedEquals(
        const Block &other, bool colorEnabled = true, bool attributeEnabled = true) const noexcept -> bool;
    /// Get a hash for this character and its color.
    [[nodiscard]] constexpr auto hash() const noexcept -> std::size_t {
        return impl::hashCreate(_character.hash(), _style.hash());
    }

public: // predefined characters.
    /// A space with inherited colors.
    [[nodiscard]] static auto space() noexcept -> const Block &;
    /// A shared empty character with inherited colors.
    [[nodiscard]] static auto empty() noexcept -> const Block &;
    /// Create an empty render cell that carries only the given style.
    /// This is mainly used for wide-character continuation cells, which must stay logically empty while preserving
    /// the visible style of the leading cell.
    /// @param style The style to store on the empty block.
    /// @return An empty block with `style`.
    [[nodiscard]] static auto emptyBlock(BlockStyle style) noexcept -> Block;

public: // deprecated methods.
    [[deprecated("Please use withOverlay(color)"), nodiscard]]
    auto withColorOverlay(const Color color) const -> Block {
        return withOverlay(color);
    }
    [[deprecated("Please use withOverlay(BlockStyle{color, attributes})"), nodiscard]]
    auto withOverlay(const Color color, const BlockAttributes attributes) const noexcept -> Block {
        return withOverlay(BlockStyle{color, attributes});
    }
    [[deprecated("Please use withBase(color)"), nodiscard]]
    auto withBaseColor(const Color color) const noexcept -> Block {
        return withBase(color);
    }
    [[deprecated("Please use withBase(BlockStyle{color, attributes})"), nodiscard]]
    auto withBase(const Color color, const BlockAttributes attributes) const noexcept -> Block {
        return withBase(BlockStyle{color, attributes});
    }

private:
    Block(const impl::CombinedBlock character, const BlockStyle style) noexcept :
        _character{character}, _style{style} {}

private:
    impl::CombinedBlock _character; ///< The Unicode character and combining code points.
    BlockStyle _style;              ///< The style for this character.
};

}

template <>
struct std::hash<erbsland::cterm::Block> {
    auto operator()(const erbsland::cterm::Block &character) const noexcept -> std::size_t { return character.hash(); }
};
