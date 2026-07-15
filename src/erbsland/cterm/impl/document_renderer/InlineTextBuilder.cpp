// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "InlineTextBuilder.hpp"

namespace erbsland::cterm::impl::document_renderer {

void InlineTextBuilder::reset() noexcept {
    _builder.clear();
    _pendingWhitespace.clear();
    _atLineStart = true;
    _semantics = {};
}

void InlineTextBuilder::appendText(const text::StringView text, const BlockStyle style, const bool preserveWhitespace) {
    if (preserveWhitespace) {
        flushPendingWhitespace();
        _builder.appendStyled(text, style);
        if (!text.isEmpty()) {
            _atLineStart = false;
        }
        return;
    }
    for (const auto character : text) {
        appendTrimmedCharacter(Block{character, style});
    }
}

void InlineTextBuilder::appendDecoration(
    const BlockStringView decoration, const BlockStyle style, const bool preserveWhitespace) {
    if (preserveWhitespace) {
        flushPendingWhitespace();
        _builder.appendWithBaseStyle(decoration, style);
        if (!decoration.isEmpty()) {
            _atLineStart = false;
        }
        return;
    }
    for (const auto &character : decoration) {
        appendTrimmedCharacter(character.withBase(style));
    }
}

void InlineTextBuilder::appendLineBreak(const BlockStyle style) {
    _pendingWhitespace.clear();
    _builder.append(Block{text::Char{U'\n'}, style});
    _atLineStart = true;
}

void InlineTextBuilder::addSoftBreak() {
    const auto index = BlockIndex::end(_builder.length());
    if (_semantics.softBreaks.empty() || _semantics.softBreaks.back() != index) {
        _semantics.softBreaks.push_back(index);
    }
}

auto InlineTextBuilder::takeString() -> BlockString {
    _pendingWhitespace.clear();
    _atLineStart = true;
    _semantics = {};
    return _builder.takeString();
}

auto InlineTextBuilder::takeContent() -> InlineContent {
    _pendingWhitespace.clear();
    _atLineStart = true;
    auto result = InlineContent{_builder.takeString(), std::move(_semantics)};
    _semantics = {};
    return result;
}

void InlineTextBuilder::addIndivisibleRange(const BlockRange range) {
    if (!range.isEmpty()) {
        _semantics.indivisibleRanges.push_back(range);
    }
}

void InlineTextBuilder::appendTrimmedCharacter(const Block &character) {
    if (character.isSpacing()) {
        if (!_atLineStart) {
            _pendingWhitespace.push_back(character);
        }
        return;
    }
    flushPendingWhitespace();
    _builder.append(character);
    _atLineStart = false;
}

void InlineTextBuilder::flushPendingWhitespace() {
    if (_pendingWhitespace.empty()) {
        return;
    }
    for (const auto &character : _pendingWhitespace) {
        _builder.append(character);
    }
    _pendingWhitespace.clear();
}

}
