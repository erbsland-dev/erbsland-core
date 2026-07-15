// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TerminalDocumentStyleMarker.hpp"

#include "impl/BlockStringBuilder.hpp"

#include "../text/String.hpp"
#include "../text/u32/U32String.hpp"

#include <string>

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

auto TerminalDocumentStyleMarker::setLiteral(const BlockStringView literal, const BlockStyle style) noexcept
    -> TerminalDocumentStyleMarker & {
    _kind = Kind::Literal;
    _literal = BlockString{literal};
    _style = style;
    return *this;
}

auto TerminalDocumentStyleMarker::setLiteral(const text::U32StringView &literal, const BlockStyle style)
    -> TerminalDocumentStyleMarker & {
    return setLiteral(BlockString{literal}, style);
}

auto TerminalDocumentStyleMarker::setLiteral(const text::StringView literal, const BlockStyle style)
    -> TerminalDocumentStyleMarker & {
    return setLiteral(BlockString{literal}, style);
}

auto TerminalDocumentStyleMarker::setOrdered() -> TerminalDocumentStyleMarker & {
    return setOrdered(BlockString{text::U32String{U".\t"}});
}

auto TerminalDocumentStyleMarker::setOrdered(const BlockStringView suffix, const BlockStyle style)
    -> TerminalDocumentStyleMarker & {
    _kind = Kind::Ordered;
    _suffix = BlockString{suffix};
    _style = style;
    return *this;
}

auto TerminalDocumentStyleMarker::setOrdered(const text::U32StringView &suffix, const BlockStyle style)
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
    const auto numberText = std::to_string(number);
    builder.appendStyled(text::String{numberText}, effectiveStyle);
    builder.appendWithBaseStyle(_suffix, effectiveStyle);
    return builder.takeString();
}

auto TerminalDocumentStyleMarker::markerStyle(const BlockStyle baseStyle) const noexcept -> BlockStyle {
    return baseStyle.withOverlay(_style);
}

}
