// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BlockString.hpp"

#include "BlockTextOptions.hpp"
#include "Font.hpp"

#include "impl/BlockStringData.hpp"
#include "impl/BlockStringRangeView.hpp"
#include "impl/BlockStringWrapper.hpp"
#include "impl/paragraph/Layout.hpp"
#include "impl/paragraph/LayoutNewlineMode.hpp"

#include <algorithm>
#include <ranges>
#include <utility>

namespace erbsland::cterm {

using bgeo::Alignment;
using bgeo::BlockCoordinate;
using bgeo::BlockSize;
using impl::BlockStringDataPtr;
using impl::BlockStringRangeView;
using impl::BlockStringWrapper;
using namespace text;
using namespace text::literals;

BlockString::BlockString() noexcept : _data{impl::sharedEmptyBlockStringData()} {
}

BlockString::BlockString(const String &string) : BlockString{BlockStringEditor{string}} {
}

BlockString::BlockString(const String &string, const BlockStyle style) : BlockString{BlockStringEditor{string, style}} {
}

BlockString::BlockString(const U32String &string) : BlockString{BlockStringEditor{string}} {
}

BlockString::BlockString(const U32String &string, const BlockStyle style) :
    BlockString{BlockStringEditor{string, style}} {
}

BlockString::BlockString(const BlockCount count, const Block character) noexcept :
    BlockString{BlockStringEditor{count, character}} {
}

BlockString::BlockString(const BlockStringEditor &string) noexcept : _data{string._data}, _range{string._range} {
}

BlockString::BlockString(BlockString &&other) noexcept : _data{std::move(other._data)}, _range{other._range} {
    other._data = impl::sharedEmptyBlockStringData();
    other._range = {};
}

auto BlockString::operator=(BlockString &&other) noexcept -> BlockString & {
    if (this != &other) {
        _data = std::move(other._data);
        _range = other._range;
        other._data = impl::sharedEmptyBlockStringData();
        other._range = {};
    }
    return *this;
}

BlockString::BlockString(BlockStringDataPtr data, const BlockRange range) noexcept :
    _data{std::move(data)}, _range{range.clampedTo(BlockCount::fromSizeT(_data->size()))} {
}

auto BlockString::operator[](const BlockIndex index) const noexcept -> Block {
    return BlockStringRangeView{*_data, _range}[index];
}

auto BlockString::displayWidth() const noexcept -> int {
    return BlockStringRangeView{*_data, _range}.displayWidth();
}

auto BlockString::at(const BlockIndex index) const -> Block {
    return BlockStringRangeView{*_data, _range}.at(index, "BlockString"_el);
}

auto BlockString::begin() const noexcept -> const_iterator {
    return BlockStringRangeView{*_data, _range}.begin();
}

auto BlockString::end() const noexcept -> const_iterator {
    return BlockStringRangeView{*_data, _range}.end();
}

auto BlockString::cbegin() const noexcept -> const_iterator {
    return BlockStringRangeView{*_data, _range}.cbegin();
}

auto BlockString::cend() const noexcept -> const_iterator {
    return BlockStringRangeView{*_data, _range}.cend();
}

auto BlockString::rbegin() const noexcept -> const_reverse_iterator {
    return BlockStringRangeView{*_data, _range}.rbegin();
}

auto BlockString::rend() const noexcept -> const_reverse_iterator {
    return BlockStringRangeView{*_data, _range}.rend();
}

auto BlockString::crbegin() const noexcept -> const_reverse_iterator {
    return BlockStringRangeView{*_data, _range}.crbegin();
}

auto BlockString::crend() const noexcept -> const_reverse_iterator {
    return BlockStringRangeView{*_data, _range}.crend();
}

auto BlockString::count(const Block &character) const noexcept -> BlockCount {
    return BlockStringRangeView{*_data, _range}.count(character);
}

auto BlockString::count(const Char character) const noexcept -> BlockCount {
    return BlockStringRangeView{*_data, _range}.count(character);
}

auto BlockString::indexOf(const Block &character, const BlockIndex startIndex) const noexcept -> BlockIndex {
    return BlockStringRangeView{*_data, _range}.indexOf(character, startIndex);
}

auto BlockString::indexOf(const Char character, const BlockIndex startIndex) const noexcept -> BlockIndex {
    return BlockStringRangeView{*_data, _range}.indexOf(character, startIndex);
}

auto BlockString::indexOf(const CharSet &characterSet, const BlockIndex startIndex) const noexcept -> BlockIndex {
    return BlockStringRangeView{*_data, _range}.indexOf(characterSet, startIndex);
}

auto BlockString::indexNotOf(const CharSet &characterSet, const BlockIndex startIndex) const noexcept -> BlockIndex {
    return BlockStringRangeView{*_data, _range}.indexNotOf(characterSet, startIndex);
}

auto BlockString::slice(const BlockRange range) const noexcept -> BlockString {
    const auto storageRange = BlockStringRangeView{*_data, _range}.subRange(range);
    if (storageRange.isEmpty()) {
        return {};
    }
    return BlockString{_data, storageRange};
}

auto BlockString::slice(const StringSide side, const BlockCount count) const noexcept -> BlockString {
    if (side == StringSide::Back) {
        if (count >= length()) {
            return slice();
        }
        return slice(BlockRange{BlockIndex::end(length() - count), count});
    }
    return slice(BlockRange{BlockIndex::zero(), count});
}

auto BlockString::croppedToDisplayWidth(const BlockCoordinate displayWidth, const Alignment alignment) const noexcept
    -> BlockString {
    const auto range = BlockStringRangeView{*_data, _range}.croppedRange(displayWidth, alignment);
    if (range.isEmpty()) {
        return {};
    }
    return BlockString{_data, range};
}

auto BlockString::trimmed(const CharSet &characters) const noexcept -> BlockString {
    const auto range = BlockStringRangeView{*_data, _range}.trimmedRange(characters);
    if (range.isEmpty()) {
        return {};
    }
    return BlockString{_data, range};
}

auto BlockString::containsControlCharacters() const noexcept -> bool {
    return BlockStringRangeView{*_data, _range}.containsControlCharacters();
}

auto BlockString::splitWords() const noexcept -> std::vector<BlockString> {
    auto words = std::vector<BlockString>{};
    for (const auto &range : BlockStringRangeView{*_data, _range}.splitWordRanges()) {
        words.emplace_back(BlockString{_data, range});
    }
    return words;
}

auto BlockString::wrapIntoLines(const int width, const ParagraphSpacing paragraphSpacing) const noexcept
    -> BlockStringLines {
    const auto editorLines = BlockStringWrapper{*this}.wrapIntoLines(width, paragraphSpacing);
    auto lines = BlockStringLines{};
    lines.reserve(editorLines.size());
    for (const auto &line : editorLines) {
        lines.emplace_back(line);
    }
    return lines;
}

auto BlockString::terminalLines(const int width) const noexcept -> int {
    return BlockStringRangeView{*_data, _range}.terminalLines(width);
}

auto BlockString::naturalBlockTextSize() const noexcept -> BlockSize {
    return BlockStringRangeView{*_data, _range}.naturalBlockTextSize();
}

auto BlockString::wrappedBlockTextHeight(const BlockCoordinate width, const BlockTextOptions &options) const noexcept
    -> BlockCoordinate {
    const auto margins = options.margins();
    const auto verticalMargins = margins.verticalExtent();
    if (options.font() != nullptr) {
        auto lineCount = BlockCoordinate{0};
        const auto lineHeight = std::max(BlockCoordinate{1}, BlockCoordinate{(options.font()->height() + 1) / 2});
        for (const auto &line : splitLines()) {
            if (std::ranges::any_of(line, [&](const Block &character) -> bool {
                    return options.font()->glyph(character.toString()) != nullptr;
                })) {
                lineCount += lineHeight;
            }
        }
        return std::max(lineCount, BlockCoordinate{1}) + verticalMargins;
    }
    const auto contentWidth = std::max(width - margins.horizontalExtent(), BlockCoordinate{1});
    const auto layout =
        impl::paragraph::Layout{
            *this,
            contentWidth.toRawValue(),
            options.paragraphOptions(),
            impl::paragraph::LayoutNewlineMode::ParagraphBreak}
            .build();
    if (layout.valid()) {
        return std::max(BlockCoordinate{layout.size()}, BlockCoordinate{1}) + verticalMargins;
    }
    if (options.onError() == ParagraphOnError::Empty) {
        return verticalMargins;
    }
    const auto fallbackLines =
        BlockStringWrapper{*this}.wrapIntoLines(contentWidth.toRawValue(), options.paragraphSpacing());
    return std::max(BlockCoordinate{fallbackLines.size()}, BlockCoordinate{1}) + verticalMargins;
}

auto BlockString::splitLines() const noexcept -> std::vector<BlockString> {
    auto result = std::vector<BlockString>{};
    for (const auto &range : BlockStringRangeView{*_data, _range}.splitLineRanges()) {
        if (range.isEmpty()) {
            result.emplace_back();
        } else {
            result.emplace_back(BlockString{_data, range});
        }
    }
    return result;
}

auto BlockString::withBase(const BlockStyle style) const noexcept -> BlockString {
    return BlockStringEditor{*this}.withBase(style);
}

auto BlockString::fromLines(
    const std::initializer_list<String> lines, const Color color, const BlockAttributes attributes) noexcept
    -> BlockString {
    return BlockStringEditor::fromLines(lines, color, attributes);
}

auto BlockString::fromLines(const std::initializer_list<String> lines, const BlockStyle style) noexcept -> BlockString {
    return BlockStringEditor::fromLines(lines, style);
}

auto BlockString::fromLines(
    const std::initializer_list<U32String> lines, const Color color, const BlockAttributes attributes) noexcept
    -> BlockString {
    return BlockStringEditor::fromLines(lines, color, attributes);
}

auto BlockString::fromLines(const std::initializer_list<U32String> lines, const BlockStyle style) noexcept
    -> BlockString {
    return BlockStringEditor::fromLines(lines, style);
}

auto BlockString::characterAt(const BlockIndex localIndex) const noexcept -> const Block & {
    return BlockStringRangeView{*_data, _range}.characterAt(localIndex);
}

auto BlockString::defaultTrimCharacters() -> const CharSet & {
    static const auto characters = CharSet{{U' ', U'\n', U'\t'}};
    return characters;
}

}
