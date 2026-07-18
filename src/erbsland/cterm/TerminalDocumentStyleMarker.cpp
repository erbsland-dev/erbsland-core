// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TerminalDocumentStyleMarker.hpp"

#include "impl/BlockStringBuilder.hpp"

#include "../text/StringEditor.hpp"
#include "../text/u32/U32String.hpp"

namespace erbsland::cterm {

auto TerminalDocumentStyleMarker::clear() noexcept -> TerminalDocumentStyleMarker & {
    _kind = Kind::None;
    _literal = {};
    _suffix = {};
    _style = {};
    return *this;
}

auto TerminalDocumentStyleMarker::setStyle(const BlockStyle style) noexcept -> TerminalDocumentStyleMarker & {
    _style = style;
    return *this;
}

auto TerminalDocumentStyleMarker::setLiteral(const BlockString literal, const BlockStyle style) noexcept
    -> TerminalDocumentStyleMarker & {
    _kind = Kind::Literal;
    _literal = BlockString{literal};
    _style = style;
    return *this;
}

auto TerminalDocumentStyleMarker::setLiteral(const text::U32String &literal, const BlockStyle style)
    -> TerminalDocumentStyleMarker & {
    return setLiteral(BlockString{literal}, style);
}

auto TerminalDocumentStyleMarker::setLiteral(const text::String &literal, const BlockStyle style)
    -> TerminalDocumentStyleMarker & {
    return setLiteral(BlockString{literal}, style);
}

auto TerminalDocumentStyleMarker::setOrdered() -> TerminalDocumentStyleMarker & {
    return setOrdered(BlockString{text::U32String{text::U32StringLiteral{U".\t"}}});
}

auto TerminalDocumentStyleMarker::setOrdered(const BlockString suffix, const BlockStyle style)
    -> TerminalDocumentStyleMarker & {
    _kind = Kind::Ordered;
    _suffix = BlockString{suffix};
    _style = style;
    return *this;
}

auto TerminalDocumentStyleMarker::setOrdered(const text::U32String &suffix, const BlockStyle style)
    -> TerminalDocumentStyleMarker & {
    return setOrdered(BlockString{suffix}, style);
}

auto TerminalDocumentStyleMarker::render(const std::size_t number, const BlockStyle baseStyle) const -> BlockString {
    const auto effectiveStyle = markerStyle(baseStyle);
    if (_kind == Kind::None) {
        return {};
    }
    if (_kind == Kind::Literal) {
        return _literal.withBase(effectiveStyle);
    }

    auto builder = impl::BlockStringBuilder{};
    builder.appendStyled(text::String::fromInteger(number), effectiveStyle);
    builder.appendWithBaseStyle(_suffix, effectiveStyle);
    return builder.takeString();
}

auto TerminalDocumentStyleMarker::markerStyle(const BlockStyle baseStyle) const noexcept -> BlockStyle {
    return baseStyle.withOverlay(_style);
}

}
