// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BlockStringView.hpp"

#include "BlockTextOptions.hpp"

#include "impl/BlockStringData.hpp"
#include "impl/BlockStringRangeView.hpp"
#include "impl/BlockStringWrapper.hpp"
#include "impl/paragraph/Layout.hpp"
#include "impl/paragraph/LayoutNewlineMode.hpp"

#include <algorithm>
#include <ranges>
#include <utility>

namespace erbsland::cterm {

BlockStringView::BlockStringView() noexcept : _data{impl::sharedEmptyBlockStringData()} {
}

BlockStringView::BlockStringView(const BlockString &string) noexcept : _data{string._data}, _range{string._range} {
}

BlockStringView::BlockStringView(BlockStringView &&other) noexcept :
    _data{std::move(other._data)}, _range{other._range} {
    other._data = impl::sharedEmptyBlockStringData();
    other._range = {};
}

auto BlockStringView::operator=(BlockStringView &&other) noexcept -> BlockStringView & {
    if (this != &other) {
        _data = std::move(other._data);
        _range = other._range;
        other._data = impl::sharedEmptyBlockStringData();
        other._range = {};
    }
    return *this;
}

BlockStringView::BlockStringView(impl::BlockStringDataPtr data, const BlockRange range) noexcept :
    _data{std::move(data)}, _range{range.clampedTo(BlockCount::fromSizeT(_data->size()))} {
}

auto BlockStringView::operator[](const BlockIndex index) const noexcept -> Block {
    return impl::BlockStringRangeView{*_data, _range}[index];
}

auto BlockStringView::displayWidth() const noexcept -> int {
    return impl::BlockStringRangeView{*_data, _range}.displayWidth();
}

auto BlockStringView::at(const BlockIndex index) const -> Block {
    return impl::BlockStringRangeView{*_data, _range}.at(index, "BlockStringView");
}

auto BlockStringView::begin() const noexcept -> const_iterator {
    return impl::BlockStringRangeView{*_data, _range}.begin();
}

auto BlockStringView::end() const noexcept -> const_iterator {
    return impl::BlockStringRangeView{*_data, _range}.end();
}

auto BlockStringView::cbegin() const noexcept -> const_iterator {
    return impl::BlockStringRangeView{*_data, _range}.cbegin();
}

auto BlockStringView::cend() const noexcept -> const_iterator {
    return impl::BlockStringRangeView{*_data, _range}.cend();
}

auto BlockStringView::rbegin() const noexcept -> const_reverse_iterator {
    return impl::BlockStringRangeView{*_data, _range}.rbegin();
}

auto BlockStringView::rend() const noexcept -> const_reverse_iterator {
    return impl::BlockStringRangeView{*_data, _range}.rend();
}

auto BlockStringView::crbegin() const noexcept -> const_reverse_iterator {
    return impl::BlockStringRangeView{*_data, _range}.crbegin();
}

auto BlockStringView::crend() const noexcept -> const_reverse_iterator {
    return impl::BlockStringRangeView{*_data, _range}.crend();
}

auto BlockStringView::count(const Block &character) const noexcept -> BlockCount {
    return impl::BlockStringRangeView{*_data, _range}.count(character);
}

auto BlockStringView::count(const text::Char character) const noexcept -> BlockCount {
    return impl::BlockStringRangeView{*_data, _range}.count(character);
}

auto BlockStringView::indexOf(const Block &character, const BlockIndex startIndex) const noexcept -> BlockIndex {
    return impl::BlockStringRangeView{*_data, _range}.indexOf(character, startIndex);
}

auto BlockStringView::indexOf(const text::Char character, const BlockIndex startIndex) const noexcept -> BlockIndex {
    return impl::BlockStringRangeView{*_data, _range}.indexOf(character, startIndex);
}

auto BlockStringView::indexOf(const text::CharSet &characterSet, const BlockIndex startIndex) const noexcept
    -> BlockIndex {
    return impl::BlockStringRangeView{*_data, _range}.indexOf(characterSet, startIndex);
}

auto BlockStringView::indexNotOf(const text::CharSet &characterSet, const BlockIndex startIndex) const noexcept
    -> BlockIndex {
    return impl::BlockStringRangeView{*_data, _range}.indexNotOf(characterSet, startIndex);
}

auto BlockStringView::slice(const BlockRange range) const noexcept -> BlockStringView {
    const auto storageRange = impl::BlockStringRangeView{*_data, _range}.subRange(range);
    if (storageRange.isEmpty()) {
        return {};
    }
    return BlockStringView{_data, storageRange};
}

auto BlockStringView::slice(const text::StringSide side, const BlockCount count) const noexcept -> BlockStringView {
    if (side == text::StringSide::Back) {
        if (count >= length()) {
            return slice();
        }
        return slice(BlockRange{BlockIndex::end(length() - count), count});
    }
    return slice(BlockRange{BlockIndex::zero(), count});
}

auto BlockStringView::croppedToDisplayWidth(
    const bgeo::BlockCoordinate displayWidth, const bgeo::Alignment alignment) const noexcept -> BlockStringView {
    const auto range = impl::BlockStringRangeView{*_data, _range}.croppedRange(displayWidth, alignment);
    if (range.isEmpty()) {
        return {};
    }
    return BlockStringView{_data, range};
}

auto BlockStringView::trimmed(const text::CharSet &characters) const noexcept -> BlockStringView {
    const auto range = impl::BlockStringRangeView{*_data, _range}.trimmedRange(characters);
    if (range.isEmpty()) {
        return {};
    }
    return BlockStringView{_data, range};
}

auto BlockStringView::containsControlCharacters() const noexcept -> bool {
    return impl::BlockStringRangeView{*_data, _range}.containsControlCharacters();
}

auto BlockStringView::splitWords() const noexcept -> std::vector<BlockStringView> {
    auto words = std::vector<BlockStringView>{};
    for (const auto &range : impl::BlockStringRangeView{*_data, _range}.splitWordRanges()) {
        words.emplace_back(BlockStringView{_data, range});
    }
    return words;
}

auto BlockStringView::wrapIntoLines(const int width, const ParagraphSpacing paragraphSpacing) const noexcept
    -> BlockStringLines {
    return impl::BlockStringWrapper{*this}.wrapIntoLines(width, paragraphSpacing);
}

auto BlockStringView::terminalLines(const int width) const noexcept -> int {
    return impl::BlockStringRangeView{*_data, _range}.terminalLines(width);
}

auto BlockStringView::naturalBlockTextSize() const noexcept -> bgeo::BlockSize {
    return impl::BlockStringRangeView{*_data, _range}.naturalBlockTextSize();
}

auto BlockStringView::wrappedBlockTextHeight(
    const bgeo::BlockCoordinate width, const BlockTextOptions &options) const noexcept -> bgeo::BlockCoordinate {
    const auto margins = options.margins();
    const auto verticalMargins = margins.verticalExtent();
    if (options.font() != nullptr) {
        auto lineCount = bgeo::BlockCoordinate{0};
        const auto lineHeight =
            std::max(bgeo::BlockCoordinate{1}, bgeo::BlockCoordinate{(options.font()->height() + 1) / 2});
        for (const auto &line : splitLines()) {
            if (std::ranges::any_of(line, [&](const Block &character) -> bool {
                    return options.font()->glyph(character.toString()) != nullptr;
                })) {
                lineCount += lineHeight;
            }
        }
        return std::max(lineCount, bgeo::BlockCoordinate{1}) + verticalMargins;
    }
    const auto contentWidth = std::max(width - margins.horizontalExtent(), bgeo::BlockCoordinate{1});
    const auto layout =
        impl::paragraph::Layout{
            *this,
            contentWidth.toRawValue(),
            options.paragraphOptions(),
            impl::paragraph::LayoutNewlineMode::ParagraphBreak}
            .build();
    if (layout.valid()) {
        return std::max(bgeo::BlockCoordinate{layout.size()}, bgeo::BlockCoordinate{1}) + verticalMargins;
    }
    if (options.onError() == ParagraphOnError::Empty) {
        return verticalMargins;
    }
    const auto fallbackLines =
        impl::BlockStringWrapper{*this}.wrapIntoLines(contentWidth.toRawValue(), options.paragraphSpacing());
    return std::max(bgeo::BlockCoordinate{fallbackLines.size()}, bgeo::BlockCoordinate{1}) + verticalMargins;
}

auto BlockStringView::splitLines() const noexcept -> std::vector<BlockStringView> {
    auto result = std::vector<BlockStringView>{};
    for (const auto &range : impl::BlockStringRangeView{*_data, _range}.splitLineRanges()) {
        if (range.isEmpty()) {
            result.emplace_back();
        } else {
            result.emplace_back(BlockStringView{_data, range});
        }
    }
    return result;
}

auto BlockStringView::withBase(const BlockStyle style) const noexcept -> BlockString {
    return BlockString{*this}.withBase(style);
}

auto BlockStringView::characterAt(const BlockIndex localIndex) const noexcept -> const Block & {
    return impl::BlockStringRangeView{*_data, _range}.characterAt(localIndex);
}

auto BlockStringView::defaultTrimCharacters() -> const text::CharSet & {
    static const auto characters = text::CharSet{{U' ', U'\n', U'\t'}};
    return characters;
}

}
