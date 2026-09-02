// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BlockStringEditor.hpp"

#include "BlockString.hpp"

#include "impl/BlockPrintContextToBlockString.hpp"
#include "impl/BlockPrintContextToString.hpp"
#include "impl/BlockStringData.hpp"
#include "impl/BlockStringRangeView.hpp"
#include "impl/BlockStringWrapper.hpp"

#include <algorithm>
#include <span>
#include <utility>

namespace erbsland::cterm {

using namespace impl;

auto BlockStringEditor::splitWords() const noexcept -> std::vector<BlockStringEditor> {
    auto words = std::vector<BlockStringEditor>{};
    for (const auto &word : BlockString{*this}.splitWords()) {
        words.push_back(BlockStringEditor{word._data, word._range});
    }
    return words;
}

auto BlockStringEditor::wrapIntoLines(const int width, const ParagraphSpacing paragraphSpacing) const noexcept
    -> std::vector<BlockStringEditor> {
    return BlockStringWrapper{*this}.wrapIntoLines(width, paragraphSpacing);
}

auto BlockStringEditor::terminalLines(const int width) const noexcept -> int {
    return BlockString{*this}.terminalLines(width);
}

auto BlockStringEditor::naturalBlockTextSize() const noexcept -> block::Size {
    return BlockString{*this}.naturalBlockTextSize();
}

auto BlockStringEditor::wrappedBlockTextHeight(
    const block::Coordinate width, const BlockTextOptions &options) const noexcept -> block::Coordinate {
    return BlockString{*this}.wrappedBlockTextHeight(width, options);
}

auto BlockStringEditor::splitLines() const noexcept -> std::vector<BlockStringEditor> {
    auto result = std::vector<BlockStringEditor>{};
    for (const auto &line : BlockString{*this}.splitLines()) {
        result.push_back(BlockStringEditor{line._data, line._range});
    }
    return result;
}

auto BlockStringEditor::withBase(const BlockStyle style) const noexcept -> BlockStringEditor {
    // This method does not detach, if the base style keeps the original string intact.
    if (style == BlockStyle{}) { // fast-path
        return *this;
    }
    auto result = *this;
    // here we first compare and only assign actual changed characters.
    for (std::size_t i = 0; i < result.length().toSizeT(); ++i) {
        const auto index = BlockIndex::fromSizeT(i);
        const auto character = result.at(index);
        const auto styledCharacter = character.withBase(style);
        if (character != styledCharacter) {
            result[index] = styledCharacter;
        }
    }
    return result;
}

auto BlockStringEditor::fromLines(
    const std::initializer_list<text::String> lines, const Color color, const BlockAttributes attributes) noexcept
    -> BlockStringEditor {
    if (lines.size() == 0) {
        return {};
    }
    auto data = BlockStringData{};
    auto reservedCapacity = lines.size() - 1U; // count newlines
    for (auto &line : lines) {
        reservedCapacity += static_cast<std::size_t>(BlockStringData::measureDisplayWidth(line));
    }
    data.reserve(reservedCapacity);
    bool isFirstLine = true;
    for (const auto &line : lines) {
        if (isFirstLine) {
            isFirstLine = false;
        } else {
            data.append(Block{U'\n', color, attributes});
        }
        data.appendCharacters(line, color, attributes);
    }
    return fromStorageWithDisplayWidth(std::move(data.chars()), data.displayWidth());
}

auto BlockStringEditor::fromLines(const std::initializer_list<text::String> lines, const BlockStyle style) noexcept
    -> BlockStringEditor {
    return fromLines(lines, style.color(), style.attributes());
}

auto BlockStringEditor::fromLines(
    const std::initializer_list<text::U32String> lines, const Color color, const BlockAttributes attributes) noexcept
    -> BlockStringEditor {
    if (lines.size() == 0) {
        return {};
    }
    auto data = BlockStringData{};
    auto reservedCapacity = lines.size() - 1U; // count newlines
    for (auto &line : lines) {
        reservedCapacity += static_cast<std::size_t>(BlockStringData::measureDisplayWidth(line));
    }
    data.reserve(reservedCapacity);
    bool isFirstLine = true;
    for (const auto &line : lines) {
        if (isFirstLine) {
            isFirstLine = false;
        } else {
            data.append(Block{U'\n', color, attributes});
        }
        data.appendCharacters(line, color, attributes);
    }
    return fromStorageWithDisplayWidth(std::move(data.chars()), data.displayWidth());
}

auto BlockStringEditor::fromLines(const std::initializer_list<text::U32String> lines, const BlockStyle style) noexcept
    -> BlockStringEditor {
    return fromLines(lines, style.color(), style.attributes());
}

auto BlockStringEditor::splitCharacters(const text::String &str, const Color color, const BlockAttributes attributes)
    -> Storage {
    auto data = BlockStringData{};
    data.reserve(static_cast<std::size_t>(BlockStringData::measureDisplayWidth(str)));
    data.appendCharacters(str, color, attributes);
    return std::move(data.chars());
}

auto BlockStringEditor::splitCharacters(const text::U32String &str, const Color color, const BlockAttributes attributes)
    -> Storage {
    auto data = BlockStringData{};
    data.reserve(str.length().toSizeT());
    data.appendCharacters(str, color, attributes);
    return std::move(data.chars());
}

auto BlockStringEditor::createPrintContext() noexcept -> BlockPrintContextPtr {
    return std::make_unique<BlockPrintContextToBlockString>(*this);
}

void BlockStringEditor::appendString(const BlockString &view, const BlockStyle) noexcept {
    if (view.isEmpty()) {
        return;
    }
    detach();
    auto &chars = _data->chars();
    chars.insert(chars.end(), view.begin(), view.end());
    _data->addDisplayWidth(view.displayWidth());
    syncRangeWithStorage();
}

void BlockStringEditor::appendStringWithBaseStyle(const BlockString &view, const BlockStyle style) noexcept {
    if (view.isEmpty()) {
        return;
    }
    detach();
    for (const auto &character : view) {
        const auto resolved = character.withBase(style);
        _data->append(resolved);
    }
    syncRangeWithStorage();
}

void BlockStringEditor::detach() {
    if (_range.index().isZero() && _range.length().toSizeT() == _data->size() && _data.useCount() == 1) {
        return;
    }
    _data = BlockStringDataPtr{new BlockStringData{_data->copyChars(_range)}};
    _range = BlockRange{BlockIndex::zero(), BlockCount::fromSizeT(_data->size())};
}

auto BlockStringEditor::fromStorageWithDisplayWidth(Storage chars, const int displayWidth) noexcept
    -> BlockStringEditor {
    if (chars.empty()) {
        return {};
    }
    const auto size = chars.size();
    auto data = BlockStringDataPtr{new BlockStringData{std::move(chars), displayWidth}};
    return BlockStringEditor{std::move(data), BlockRange{BlockIndex::zero(), BlockCount::fromSizeT(size)}};
}

void BlockStringEditor::syncRangeWithStorage() noexcept {
    _range = BlockRange{BlockIndex::zero(), BlockCount::fromSizeT(_data->size())};
}

auto BlockStringEditor::characterAt(const BlockIndex index) const noexcept -> const Block & {
    return BlockStringRangeView{*_data, _range}.characterAt(index);
}

auto BlockStringEditor::defaultTrimCharacters() -> const text::CharSet & {
    static const auto characters = text::CharSet{{U' ', U'\n', U'\t'}};
    return characters;
}

auto BlockStringEditor::ignoredMutableCharacter() noexcept -> Block & {
    static thread_local auto ignored = Block{};
    ignored = Block{};
    return ignored;
}

}
