// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TerminalDocumentStyleRule.hpp"

#include "../text/u32/U32StringEditor.hpp"

namespace erbsland::cterm {

auto TerminalDocumentStyleRule::setTextStyle(const BlockStyle style) noexcept -> TerminalDocumentStyleRule & {
    _textStyle = style;
    return *this;
}

auto TerminalDocumentStyleRule::prefix() const noexcept -> std::optional<BlockString> {
    if (!_prefix.has_value()) {
        return std::nullopt;
    }
    return BlockString{*_prefix};
}

auto TerminalDocumentStyleRule::suffix() const noexcept -> std::optional<BlockString> {
    if (!_suffix.has_value()) {
        return std::nullopt;
    }
    return BlockString{*_suffix};
}

auto TerminalDocumentStyleRule::linePrefix() const noexcept -> std::optional<BlockString> {
    if (!_linePrefix.has_value()) {
        return std::nullopt;
    }
    return BlockString{*_linePrefix};
}

auto TerminalDocumentStyleRule::setTextStyle(const Color color, const BlockAttributes attributes) noexcept
    -> TerminalDocumentStyleRule & {
    return setTextStyle(BlockStyle{color, attributes});
}

auto TerminalDocumentStyleRule::setIndents(const ParagraphIndents indents) noexcept -> TerminalDocumentStyleRule & {
    _indents = indents;
    return *this;
}

auto TerminalDocumentStyleRule::setMargins(const bgeo::BlockMargins margins) noexcept -> TerminalDocumentStyleRule & {
    _indents.setMargins(margins);
    return *this;
}

auto TerminalDocumentStyleRule::setMargins(const int allSides) noexcept -> TerminalDocumentStyleRule & {
    return setMargins(bgeo::BlockMargins{allSides});
}

auto TerminalDocumentStyleRule::setMargins(const int horizontal, const int vertical) noexcept
    -> TerminalDocumentStyleRule & {
    return setMargins(bgeo::BlockMargins{horizontal, vertical});
}

auto TerminalDocumentStyleRule::setMargins(const int top, const int right, const int bottom, const int left) noexcept
    -> TerminalDocumentStyleRule & {
    return setMargins(bgeo::BlockMargins{top, right, bottom, left});
}

auto TerminalDocumentStyleRule::setLineIndent(const int indent) noexcept -> TerminalDocumentStyleRule & {
    _indents.setLineIndent(indent);
    return *this;
}

auto TerminalDocumentStyleRule::setFirstLineIndent(const int indent) noexcept -> TerminalDocumentStyleRule & {
    _indents.setFirstLineIndent(indent);
    return *this;
}

auto TerminalDocumentStyleRule::setWrappedLineIndent(const int indent) noexcept -> TerminalDocumentStyleRule & {
    _indents.setWrappedLineIndent(indent);
    return *this;
}

auto TerminalDocumentStyleRule::setPrefix(const BlockString prefix) noexcept -> TerminalDocumentStyleRule & {
    _prefix = prefix;
    return *this;
}

auto TerminalDocumentStyleRule::setPrefix(const text::U32String &prefix, const BlockStyle style)
    -> TerminalDocumentStyleRule & {
    return setPrefix(BlockString{prefix, style});
}

auto TerminalDocumentStyleRule::setPrefix(const text::String &prefix, const BlockStyle style)
    -> TerminalDocumentStyleRule & {
    return setPrefix(BlockString{prefix, style});
}

auto TerminalDocumentStyleRule::clearPrefix() noexcept -> TerminalDocumentStyleRule & {
    _prefix.reset();
    return *this;
}

auto TerminalDocumentStyleRule::setSuffix(const BlockString suffix) noexcept -> TerminalDocumentStyleRule & {
    _suffix = suffix;
    return *this;
}

auto TerminalDocumentStyleRule::setSuffix(const text::U32String &suffix, const BlockStyle style)
    -> TerminalDocumentStyleRule & {
    return setSuffix(BlockString{suffix, style});
}

auto TerminalDocumentStyleRule::setSuffix(const text::String &suffix, const BlockStyle style)
    -> TerminalDocumentStyleRule & {
    return setSuffix(BlockString{suffix, style});
}

auto TerminalDocumentStyleRule::clearSuffix() noexcept -> TerminalDocumentStyleRule & {
    _suffix.reset();
    return *this;
}

auto TerminalDocumentStyleRule::setLinePrefix(const BlockString prefix) noexcept -> TerminalDocumentStyleRule & {
    _linePrefix = prefix;
    return *this;
}

auto TerminalDocumentStyleRule::setLinePrefix(const text::U32String &prefix, const BlockStyle style)
    -> TerminalDocumentStyleRule & {
    return setLinePrefix(BlockString{prefix, style});
}

auto TerminalDocumentStyleRule::setLinePrefix(const text::String &prefix, const BlockStyle style)
    -> TerminalDocumentStyleRule & {
    return setLinePrefix(BlockString{prefix, style});
}

auto TerminalDocumentStyleRule::clearLinePrefix() noexcept -> TerminalDocumentStyleRule & {
    _linePrefix.reset();
    return *this;
}

auto TerminalDocumentStyleRule::setLineFill(const Block fill) noexcept -> TerminalDocumentStyleRule & {
    _lineFill = fill;
    return *this;
}

auto TerminalDocumentStyleRule::setLineFill(const text::Char codePoint, const BlockStyle style) noexcept
    -> TerminalDocumentStyleRule & {
    return setLineFill(Block{codePoint, style});
}

auto TerminalDocumentStyleRule::clearLineFill() noexcept -> TerminalDocumentStyleRule & {
    _lineFill.reset();
    return *this;
}

auto TerminalDocumentStyleRule::setMarker(TerminalDocumentStyleMarker marker) noexcept -> TerminalDocumentStyleRule & {
    _marker = std::move(marker);
    return *this;
}

auto TerminalDocumentStyleRule::setLiteralMarker(const BlockString literal, const BlockStyle style)
    -> TerminalDocumentStyleRule & {
    _marker.setLiteral(literal, style);
    return *this;
}

auto TerminalDocumentStyleRule::setLiteralMarker(const text::U32String &literal, const BlockStyle style)
    -> TerminalDocumentStyleRule & {
    _marker.setLiteral(literal, style);
    return *this;
}

auto TerminalDocumentStyleRule::setOrderedMarker(const BlockString suffix, const BlockStyle style)
    -> TerminalDocumentStyleRule & {
    _marker.setOrdered(suffix, style);
    return *this;
}

auto TerminalDocumentStyleRule::setOrderedMarker() -> TerminalDocumentStyleRule & {
    _marker.setOrdered();
    return *this;
}

auto TerminalDocumentStyleRule::clearMarker() noexcept -> TerminalDocumentStyleRule & {
    _marker.clear();
    return *this;
}

}
