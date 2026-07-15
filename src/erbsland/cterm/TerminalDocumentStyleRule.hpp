// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Block.hpp"
#include "BlockString.hpp"
#include "BlockStringView.hpp"
#include "BlockStyle.hpp"
#include "ParagraphIndents.hpp"
#include "TerminalDocumentStyleMarker.hpp"

#include "../bgeo/BlockMargins.hpp"
#include "../text/StringView.hpp"
#include "../text/u32/U32StringView.hpp"

#include <optional>

namespace erbsland::cterm {

/// One terminal document style rule with text, layout, and decoration settings.
/// @tested{TerminalDocumentStyleTest}
class TerminalDocumentStyleRule final {
public:
    /// Create an empty style rule.
    TerminalDocumentStyleRule() = default;

    // defaults
    ~TerminalDocumentStyleRule() = default;
    TerminalDocumentStyleRule(const TerminalDocumentStyleRule &) = default;
    TerminalDocumentStyleRule(TerminalDocumentStyleRule &&) = default;
    auto operator=(const TerminalDocumentStyleRule &) -> TerminalDocumentStyleRule & = default;
    auto operator=(TerminalDocumentStyleRule &&) -> TerminalDocumentStyleRule & = default;

public: // accessors
    /// Get the text style overlay.
    [[nodiscard]] auto textStyle() const noexcept -> BlockStyle { return _textStyle; }
    /// Get the paragraph indents and margins.
    [[nodiscard]] auto indents() const noexcept -> const ParagraphIndents & { return _indents; }
    /// Get the margins around the block.
    [[nodiscard]] auto margins() const noexcept -> const bgeo::BlockMargins & { return _indents.margins(); }
    /// Get the optional prefix.
    [[nodiscard]] auto prefix() const noexcept -> std::optional<BlockStringView>;
    /// Get the optional suffix.
    [[nodiscard]] auto suffix() const noexcept -> std::optional<BlockStringView>;
    /// Get the optional prefix applied to every content line of a container.
    [[nodiscard]] auto linePrefix() const noexcept -> std::optional<BlockStringView>;
    /// Get the optional line fill character.
    [[nodiscard]] auto lineFill() const noexcept -> const std::optional<Block> & { return _lineFill; }
    /// Get the marker configuration.
    [[nodiscard]] auto marker() noexcept -> TerminalDocumentStyleMarker & { return _marker; }
    /// Get the marker configuration.
    [[nodiscard]] auto marker() const noexcept -> const TerminalDocumentStyleMarker & { return _marker; }

public:
    /// Replace the text style overlay.
    /// @param style The new text style overlay.
    /// @return Reference to this rule.
    auto setTextStyle(BlockStyle style) noexcept -> TerminalDocumentStyleRule &;
    /// Replace the text style overlay.
    /// @param color The new color overlay.
    /// @param attributes The new attribute overlay.
    /// @return Reference to this rule.
    auto setTextStyle(Color color, BlockAttributes attributes = {}) noexcept -> TerminalDocumentStyleRule &;
    /// Replace the indents and margins.
    /// @param indents The new indents and margins.
    /// @return Reference to this rule.
    auto setIndents(ParagraphIndents indents) noexcept -> TerminalDocumentStyleRule &;
    /// Replace the margins.
    /// @param margins The new margins.
    /// @return Reference to this rule.
    auto setMargins(bgeo::BlockMargins margins) noexcept -> TerminalDocumentStyleRule &;
    /// Replace all margins with one value.
    /// @param allSides The value for all sides.
    /// @return Reference to this rule.
    auto setMargins(int allSides) noexcept -> TerminalDocumentStyleRule &;
    /// Replace horizontal and vertical margins.
    /// @param horizontal The left and right margins.
    /// @param vertical The top and bottom margins.
    /// @return Reference to this rule.
    auto setMargins(int horizontal, int vertical) noexcept -> TerminalDocumentStyleRule &;
    /// Replace each margin side.
    /// @param top Top margin.
    /// @param right Right margin.
    /// @param bottom Bottom margin.
    /// @param left Left margin.
    /// @return Reference to this rule.
    auto setMargins(int top, int right, int bottom, int left) noexcept -> TerminalDocumentStyleRule &;
    /// Set the shared line indent.
    /// @param indent The new line indent.
    /// @return Reference to this rule.
    auto setLineIndent(int indent) noexcept -> TerminalDocumentStyleRule &;
    /// Set the first-line indent.
    /// @param indent The new first-line indent.
    /// @return Reference to this rule.
    auto setFirstLineIndent(int indent) noexcept -> TerminalDocumentStyleRule &;
    /// Set the wrapped-line indent.
    /// @param indent The new wrapped-line indent.
    /// @return Reference to this rule.
    auto setWrappedLineIndent(int indent) noexcept -> TerminalDocumentStyleRule &;
    /// Set the optional prefix.
    /// @param prefix The prefix text.
    /// @return Reference to this rule.
    auto setPrefix(BlockStringView prefix) noexcept -> TerminalDocumentStyleRule &;
    /// Set the optional prefix.
    /// @param prefix The prefix text.
    /// @param style The prefix style.
    /// @return Reference to this rule.
    auto setPrefix(const text::U32StringView &prefix, BlockStyle style = {}) -> TerminalDocumentStyleRule &;
    /// Set the optional prefix.
    /// @param prefix The prefix text.
    /// @param style The prefix style.
    /// @return Reference to this rule.
    auto setPrefix(text::StringView prefix, BlockStyle style = {}) -> TerminalDocumentStyleRule &;
    /// Remove the prefix.
    /// @return Reference to this rule.
    auto clearPrefix() noexcept -> TerminalDocumentStyleRule &;
    /// Set the optional suffix.
    /// @param suffix The suffix text.
    /// @return Reference to this rule.
    auto setSuffix(BlockStringView suffix) noexcept -> TerminalDocumentStyleRule &;
    /// Set the optional suffix.
    /// @param suffix The suffix text.
    /// @param style The suffix style.
    /// @return Reference to this rule.
    auto setSuffix(const text::U32StringView &suffix, BlockStyle style = {}) -> TerminalDocumentStyleRule &;
    /// Set the optional suffix.
    /// @param suffix The suffix text.
    /// @param style The suffix style.
    /// @return Reference to this rule.
    auto setSuffix(text::StringView suffix, BlockStyle style = {}) -> TerminalDocumentStyleRule &;
    /// Remove the suffix.
    /// @return Reference to this rule.
    auto clearSuffix() noexcept -> TerminalDocumentStyleRule &;
    /// Set the prefix applied to every content line of a container.
    auto setLinePrefix(BlockStringView prefix) noexcept -> TerminalDocumentStyleRule &;
    /// Set the styled prefix applied to every content line of a container.
    auto setLinePrefix(const text::U32StringView &prefix, BlockStyle style = {}) -> TerminalDocumentStyleRule &;
    /// Set the styled prefix applied to every content line of a container.
    auto setLinePrefix(text::StringView prefix, BlockStyle style = {}) -> TerminalDocumentStyleRule &;
    /// Remove the per-line prefix.
    auto clearLinePrefix() noexcept -> TerminalDocumentStyleRule &;
    /// Set the line fill character.
    /// @param fill The fill character.
    /// @return Reference to this rule.
    auto setLineFill(Block fill) noexcept -> TerminalDocumentStyleRule &;
    /// Set the line fill character.
    /// @param codePoint The fill code point.
    /// @param style The fill style.
    /// @return Reference to this rule.
    auto setLineFill(text::Char codePoint, BlockStyle style = {}) noexcept -> TerminalDocumentStyleRule &;
    /// Remove the line fill character.
    /// @return Reference to this rule.
    auto clearLineFill() noexcept -> TerminalDocumentStyleRule &;
    /// Replace the marker.
    /// @param marker The marker configuration.
    /// @return Reference to this rule.
    auto setMarker(TerminalDocumentStyleMarker marker) noexcept -> TerminalDocumentStyleRule &;
    /// Configure a literal marker.
    /// @param literal The marker text.
    /// @param style Optional marker style.
    /// @return Reference to this rule.
    auto setLiteralMarker(BlockStringView literal, BlockStyle style = {}) -> TerminalDocumentStyleRule &;
    /// Configure a literal marker.
    /// @param literal The marker text.
    /// @param style Optional marker style.
    /// @return Reference to this rule.
    auto setLiteralMarker(const text::U32StringView &literal, BlockStyle style = {}) -> TerminalDocumentStyleRule &;
    /// Configure an ordered marker.
    /// @param suffix The marker suffix.
    /// @param style Optional marker style.
    /// @return Reference to this rule.
    auto setOrderedMarker(BlockStringView suffix, BlockStyle style = {}) -> TerminalDocumentStyleRule &;
    /// Configure the default ordered marker.
    /// @return Reference to this rule.
    auto setOrderedMarker() -> TerminalDocumentStyleRule &;
    /// Clear the marker.
    /// @return Reference to this rule.
    auto clearMarker() noexcept -> TerminalDocumentStyleRule &;

private:
    BlockStyle _textStyle;                  ///< Text style overlay.
    ParagraphIndents _indents;              ///< Block layout.
    std::optional<BlockString> _prefix;     ///< Optional prefix storage.
    std::optional<BlockString> _suffix;     ///< Optional suffix storage.
    std::optional<BlockString> _linePrefix; ///< Optional per-line container prefix.
    std::optional<Block> _lineFill;         ///< Optional fill character.
    TerminalDocumentStyleMarker _marker;    ///< Optional list marker.
};

}
