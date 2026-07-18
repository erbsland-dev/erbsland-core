// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BlockStringRangeView.hpp"

#include "../../err/OutOfRangeError.hpp"

#include <algorithm>
#include <ranges>
#include <string>

namespace erbsland::cterm::impl {

using text::Char;
using text::CharSet;

auto BlockStringRangeView::storageIndex(const BlockIndex localIndex) const noexcept -> BlockIndex {
    return _range.index() + localIndex.offsetFromZero();
}

auto BlockStringRangeView::rawStorageIndex(const BlockIndex localIndex) const noexcept -> std::size_t {
    return storageIndex(localIndex).toSizeT();
}

auto BlockStringRangeView::characterAt(const BlockIndex localIndex) const noexcept -> const Block & {
    return _data.chars()[rawStorageIndex(localIndex)];
}

auto BlockStringRangeView::begin() const noexcept -> const_iterator {
    return _data.chars().cbegin() + static_cast<difference_type>(_range.index().toSizeT());
}

auto BlockStringRangeView::end() const noexcept -> const_iterator {
    return begin() + static_cast<difference_type>(length().toSizeT());
}

auto BlockStringRangeView::cbegin() const noexcept -> const_iterator {
    return begin();
}

auto BlockStringRangeView::cend() const noexcept -> const_iterator {
    return end();
}

auto BlockStringRangeView::rbegin() const noexcept -> const_reverse_iterator {
    return const_reverse_iterator{end()};
}

auto BlockStringRangeView::rend() const noexcept -> const_reverse_iterator {
    return const_reverse_iterator{begin()};
}

auto BlockStringRangeView::crbegin() const noexcept -> const_reverse_iterator {
    return rbegin();
}

auto BlockStringRangeView::crend() const noexcept -> const_reverse_iterator {
    return rend();
}

auto BlockStringRangeView::displayWidth() const noexcept -> int {
    if (_range.index().isZero() && length().toSizeT() == _data.size() && _data.hasDisplayWidthCache()) {
        return _data.displayWidth();
    }
    auto result = 0;
    for (auto it = begin(); it != end(); ++it) {
        result += it->displayWidth();
    }
    return result;
}

auto BlockStringRangeView::operator[](const BlockIndex index) const noexcept -> Block {
    if (!index.isWithin(length())) {
        return {};
    }
    return characterAt(index);
}

auto BlockStringRangeView::at(const BlockIndex index, const std::string_view typeName) const -> Block {
    if (!index.isWithin(length())) {
        throw err::OutOfRangeError{std::string{typeName} + " index out of range."};
    }
    return characterAt(index);
}

auto BlockStringRangeView::count(const Block &character) const noexcept -> BlockCount {
    return BlockCount::fromSizeT(
        static_cast<std::size_t>(
            std::ranges::count_if(*this, [&](const Block &candidate) -> bool { return candidate == character; })));
}

auto BlockStringRangeView::count(const Char character) const noexcept -> BlockCount {
    return BlockCount::fromSizeT(
        static_cast<std::size_t>(std::ranges::count_if(*this, [&](const Block &candidate) -> bool {
            return candidate.singleOrNull() == character.toRawValue();
        })));
}

auto BlockStringRangeView::indexOf(const Block &character, const BlockIndex startIndex) const noexcept -> BlockIndex {
    if (!startIndex.isWithin(length())) {
        return BlockIndex::noIndex();
    }
    for (auto index = startIndex.toSizeT(); index < rawSize(); ++index) {
        if (characterAt(BlockIndex::fromSizeT(index)) == character) {
            return BlockIndex::fromSizeT(index);
        }
    }
    return BlockIndex::noIndex();
}

auto BlockStringRangeView::indexOf(const Char character, const BlockIndex startIndex) const noexcept -> BlockIndex {
    if (!startIndex.isWithin(length())) {
        return BlockIndex::noIndex();
    }
    for (auto index = startIndex.toSizeT(); index < rawSize(); ++index) {
        if (characterAt(BlockIndex::fromSizeT(index)).singleOrNull() == character.toRawValue()) {
            return BlockIndex::fromSizeT(index);
        }
    }
    return BlockIndex::noIndex();
}

auto BlockStringRangeView::indexOf(const CharSet &characterSet, const BlockIndex startIndex) const noexcept
    -> BlockIndex {
    if (!startIndex.isWithin(length())) {
        return BlockIndex::noIndex();
    }
    for (auto index = startIndex.toSizeT(); index < rawSize(); ++index) {
        if (characterSet.contains(Char{characterAt(BlockIndex::fromSizeT(index)).singleOrNull()})) {
            return BlockIndex::fromSizeT(index);
        }
    }
    return BlockIndex::noIndex();
}

auto BlockStringRangeView::indexNotOf(const CharSet &characterSet, const BlockIndex startIndex) const noexcept
    -> BlockIndex {
    if (!startIndex.isWithin(length())) {
        return BlockIndex::noIndex();
    }
    for (auto index = startIndex.toSizeT(); index < rawSize(); ++index) {
        if (!characterSet.contains(Char{characterAt(BlockIndex::fromSizeT(index)).singleOrNull()})) {
            return BlockIndex::fromSizeT(index);
        }
    }
    return BlockIndex::noIndex();
}

auto BlockStringRangeView::subRange(const BlockRange range) const noexcept -> BlockRange {
    const auto clampedRange = range.clampedTo(length());
    if (clampedRange.isEmpty()) {
        return {};
    }
    return clampedRange.withOrigin(_range.index());
}

auto BlockStringRangeView::croppedRange(
    const bgeo::BlockCoordinate displayWidth, const bgeo::Alignment alignment) const noexcept -> BlockRange {
    if (displayWidth <= 0) {
        return {};
    }
    if (this->displayWidth() < displayWidth) {
        return _range;
    }
    auto currentDisplayWidth = bgeo::BlockCoordinate{0};
    auto index = std::size_t{0};
    auto newBlockRange = BlockRange{};
    if (alignment.isRight()) {
        index = rawSize();
        while (currentDisplayWidth < displayWidth && index > 0) {
            const auto characterDisplayWidth = characterAt(BlockIndex::fromSizeT(index - 1U)).displayWidth();
            if (currentDisplayWidth + characterDisplayWidth > displayWidth) {
                break;
            }
            currentDisplayWidth += characterDisplayWidth;
            index -= 1;
        }
        newBlockRange =
            BlockRange{storageIndex(BlockIndex::fromSizeT(index)), BlockCount::fromSizeT(rawSize() - index)};
    } else {
        while (currentDisplayWidth < displayWidth && index < rawSize()) {
            const auto characterDisplayWidth = characterAt(BlockIndex::fromSizeT(index)).displayWidth();
            if (currentDisplayWidth + characterDisplayWidth > displayWidth) {
                break;
            }
            currentDisplayWidth += characterDisplayWidth;
            index += 1;
        }
        newBlockRange = BlockRange{_range.index(), BlockCount::fromSizeT(index)};
    }
    if (newBlockRange.isEmpty()) {
        return {};
    }
    return newBlockRange;
}

auto BlockStringRangeView::trimmedRange(const CharSet &characters) const noexcept -> BlockRange {
    if (isEmpty()) {
        return {};
    }
    auto startIndex = std::size_t{0};
    auto endIndex = rawSize();
    while (
        startIndex < endIndex &&
        characters.contains(Char{characterAt(BlockIndex::fromSizeT(startIndex)).singleOrNull()})) {
        startIndex += 1;
    }
    while (
        endIndex > startIndex &&
        characters.contains(Char{characterAt(BlockIndex::fromSizeT(endIndex - 1U)).singleOrNull()})) {
        endIndex -= 1;
    }
    if (startIndex == endIndex) {
        return {};
    }
    return subRange(BlockRange{BlockIndex::fromSizeT(startIndex), BlockCount::fromSizeT(endIndex - startIndex)});
}

auto BlockStringRangeView::containsControlCharacters() const noexcept -> bool {
    return std::ranges::any_of(*this, [](const Block &character) -> bool { return character.isControl(); });
}

auto BlockStringRangeView::splitWordRanges() const noexcept -> std::vector<BlockRange> {
    auto words = std::vector<BlockRange>{};
    auto wordStart = BlockIndex::noIndex();
    for (auto index = std::size_t{0}; index < rawSize(); ++index) {
        if (characterAt(BlockIndex::fromSizeT(index)).isSpacing()) {
            if (!wordStart.isNoIndex()) {
                words.emplace_back(subRange(BlockRange{wordStart, BlockCount::fromSizeT(index - wordStart.toSizeT())}));
                wordStart = BlockIndex::noIndex();
            }
            continue;
        }
        if (wordStart.isNoIndex()) {
            wordStart = BlockIndex::fromSizeT(index);
        }
    }
    if (!wordStart.isNoIndex()) {
        words.emplace_back(subRange(BlockRange{wordStart, BlockCount::infinite()}));
    }
    return words;
}

auto BlockStringRangeView::splitLineRanges() const noexcept -> std::vector<BlockRange> {
    if (isEmpty()) {
        return {};
    }
    auto result = std::vector<BlockRange>{};
    result.reserve(count(Char{U'\n'}).toSizeT() + 1U);
    auto lineStartIndex = std::size_t{0};
    while (lineStartIndex < rawSize()) {
        const auto lineEndIndex = indexOf(Char{U'\n'}, BlockIndex::fromSizeT(lineStartIndex));
        if (lineEndIndex.isNoIndex()) {
            result.emplace_back(subRange(BlockRange{BlockIndex::fromSizeT(lineStartIndex), BlockCount::infinite()}));
            break;
        }
        result.emplace_back(subRange(
            BlockRange{
                BlockIndex::fromSizeT(lineStartIndex),
                BlockCount::fromSizeT(lineEndIndex.toSizeT() - lineStartIndex)}));
        lineStartIndex = lineEndIndex.toSizeT() + 1U;
    }
    return result;
}

auto BlockStringRangeView::terminalLines(const int width) const noexcept -> int {
    if (width <= 0) {
        return 0;
    }
    if (width == 1) {
        return static_cast<int>(rawSize());
    }
    auto renderedLines = 0;
    auto currentWidth = 0;
    for (const auto &character : *this) {
        if (character == Char{U'\n'}) {
            renderedLines += 1;
            currentWidth = 0;
            continue;
        }
        const auto characterWidth = character.displayWidth();
        currentWidth += characterWidth;
        if (currentWidth >= width) {
            renderedLines += 1;
            currentWidth = currentWidth > width ? characterWidth : 0;
        }
    }
    if (currentWidth > 0) {
        renderedLines += 1;
    }
    return renderedLines;
}

auto BlockStringRangeView::naturalBlockTextSize() const noexcept -> bgeo::BlockSize {
    auto preferredWidth = bgeo::BlockCoordinate{1};
    auto preferredHeight = bgeo::BlockCoordinate{1};
    auto currentLineWidth = bgeo::BlockCoordinate{0};
    for (auto index = std::size_t{0}; index < rawSize(); ++index) {
        const auto character = characterAt(BlockIndex::fromSizeT(index));
        if (character == Char{U'\n'}) {
            preferredWidth = std::max(preferredWidth, currentLineWidth);
            currentLineWidth = bgeo::BlockCoordinate{0};
            if (index + 1U < rawSize()) {
                preferredHeight += 1;
            }
            continue;
        }
        currentLineWidth += character.displayWidth();
    }
    preferredWidth = std::max(preferredWidth, currentLineWidth);
    return {preferredWidth, preferredHeight};
}

}
