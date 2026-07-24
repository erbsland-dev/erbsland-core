// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BlockStringEditor.hpp"

#include "BlockString.hpp"

#include "impl/BlockPrintContextToString.hpp"
#include "impl/BlockStringData.hpp"
#include "impl/BlockStringRangeView.hpp"
#include "impl/BlockStringWrapper.hpp"

#include <algorithm>
#include <span>
#include <utility>

namespace erbsland::cterm {

using impl::BlockStringData;
using impl::BlockStringDataPtr;
using impl::BlockStringRangeView;
using namespace text;
using namespace text::literals;

BlockStringEditor::BlockStringEditor() noexcept : _data{impl::sharedEmptyBlockStringData()} {
}

BlockStringEditor::BlockStringEditor(const String &str) : BlockStringEditor{str, BlockStyle{}} {
}

BlockStringEditor::BlockStringEditor(const String &str, const BlockStyle style) :
    BlockStringEditor{splitCharacters(str, style.color(), style.attributes())} {
}

BlockStringEditor::BlockStringEditor(const U32String &str) : BlockStringEditor{str, BlockStyle{}} {
}

BlockStringEditor::BlockStringEditor(const U32String &str, const BlockStyle style) :
    BlockStringEditor{splitCharacters(str, style.color(), style.attributes())} {
}

BlockStringEditor::BlockStringEditor(const BlockCount count, const Block character) noexcept :
    BlockStringEditor{Storage{std::min(count.toSizeT(), static_cast<std::size_t>(10'000'000U)), character}} {
}

BlockStringEditor::BlockStringEditor(const BlockString &view) : BlockStringEditor{view._data->copyChars(view._range)} {
}

BlockStringEditor::BlockStringEditor(BlockStringEditor &&other) noexcept :
    _data{std::move(other._data)}, _range{other._range} {
    other._data = impl::sharedEmptyBlockStringData();
    other._range = {};
}

auto BlockStringEditor::operator=(BlockStringEditor &&other) noexcept -> BlockStringEditor & {
    if (this != &other) {
        _data = std::move(other._data);
        _range = other._range;
        other._data = impl::sharedEmptyBlockStringData();
        other._range = {};
    }
    return *this;
}

BlockStringEditor::BlockStringEditor(BlockStringDataPtr data, const BlockRange range) noexcept :
    _data{std::move(data)}, _range{range.clampedTo(BlockCount::fromSizeT(_data->size()))} {
}

BlockStringEditor::BlockStringEditor(Storage chars) noexcept :
    _data{
        chars.empty() ? impl::sharedEmptyBlockStringData() : BlockStringDataPtr{new BlockStringData{std::move(chars)}}},
    _range{BlockIndex::zero(), BlockCount::fromSizeT(_data->size())} {
}

auto BlockStringEditor::operator==(const BlockStringEditor &other) const noexcept -> bool {
    const auto view = BlockStringRangeView{*_data, _range};
    const auto otherView = BlockStringRangeView{*other._data, other._range};
    if (view.length() != otherView.length()) {
        return false;
    }
    for (std::size_t i = 0; i < view.length().toSizeT(); ++i) {
        const auto index = BlockIndex::fromSizeT(i);
        if (view.characterAt(index) != otherView.characterAt(index)) {
            return false;
        }
    }
    return true;
}

auto BlockStringEditor::operator[](const BlockIndex index) const noexcept -> Block {
    return BlockStringRangeView{*_data, _range}[index];
}

auto BlockStringEditor::operator[](const BlockIndex index) noexcept -> Block & {
    if (!index.isWithin(length())) {
        return ignoredMutableCharacter();
    }
    detach();
    _data->invalidateDisplayWidth();
    return _data->chars()[index.toSizeT()];
}

auto BlockStringEditor::operator+=(const BlockStringEditor &other) noexcept -> BlockStringEditor & {
    append(other);
    return *this;
}

auto BlockStringEditor::operator+=(const BlockString &other) noexcept -> BlockStringEditor & {
    append(other);
    return *this;
}

auto BlockStringEditor::operator+(const BlockStringEditor &other) const noexcept -> BlockStringEditor {
    auto copy = *this;
    copy.append(other);
    return copy;
}

auto BlockStringEditor::operator+(const BlockString &other) const noexcept -> BlockStringEditor {
    auto copy = *this;
    copy.append(other);
    return copy;
}

auto BlockStringEditor::displayWidth() const noexcept -> int {
    return BlockStringRangeView{*_data, _range}.displayWidth();
}

auto BlockStringEditor::withTabsExpanded(const int targetColumn) const -> BlockStringEditor {
    auto result = BlockStringEditor{};
    auto column = 0;
    for (const auto &character : *this) {
        if (character.singleOrNull() != U'\t') {
            result.append(character);
            column += character.displayWidth();
            continue;
        }
        const auto count = std::max(targetColumn - column, 1);
        result.append(
            BlockStringEditor{BlockCount::fromSizeT(static_cast<std::size_t>(count)), Block{U' ', character.style()}});
        column += count;
    }
    return result;
}

auto BlockStringEditor::at(const BlockIndex index) const -> Block {
    return BlockStringRangeView{*_data, _range}.at(index, "BlockStringEditor"_el);
}

auto BlockStringEditor::begin() noexcept -> iterator {
    detach();
    _data->invalidateDisplayWidth();
    return _data->chars().begin();
}

auto BlockStringEditor::end() noexcept -> iterator {
    detach();
    _data->invalidateDisplayWidth();
    return _data->chars().end();
}

auto BlockStringEditor::begin() const noexcept -> const_iterator {
    return BlockStringRangeView{*_data, _range}.begin();
}

auto BlockStringEditor::end() const noexcept -> const_iterator {
    return BlockStringRangeView{*_data, _range}.end();
}

auto BlockStringEditor::cbegin() const noexcept -> const_iterator {
    return BlockStringRangeView{*_data, _range}.cbegin();
}

auto BlockStringEditor::cend() const noexcept -> const_iterator {
    return BlockStringRangeView{*_data, _range}.cend();
}

auto BlockStringEditor::rbegin() noexcept -> reverse_iterator {
    return reverse_iterator{end()};
}

auto BlockStringEditor::rend() noexcept -> reverse_iterator {
    return reverse_iterator{begin()};
}

auto BlockStringEditor::rbegin() const noexcept -> const_reverse_iterator {
    return BlockStringRangeView{*_data, _range}.rbegin();
}

auto BlockStringEditor::rend() const noexcept -> const_reverse_iterator {
    return BlockStringRangeView{*_data, _range}.rend();
}

auto BlockStringEditor::crbegin() const noexcept -> const_reverse_iterator {
    return BlockStringRangeView{*_data, _range}.crbegin();
}

auto BlockStringEditor::crend() const noexcept -> const_reverse_iterator {
    return BlockStringRangeView{*_data, _range}.crend();
}

auto BlockStringEditor::count(const Block &character) const noexcept -> BlockCount {
    return BlockStringRangeView{*_data, _range}.count(character);
}

auto BlockStringEditor::count(const Char character) const noexcept -> BlockCount {
    return BlockStringRangeView{*_data, _range}.count(character);
}

auto BlockStringEditor::indexOf(const Block &character, const BlockIndex startIndex) const noexcept -> BlockIndex {
    return BlockStringRangeView{*_data, _range}.indexOf(character, startIndex);
}

auto BlockStringEditor::indexOf(const Char character, const BlockIndex startIndex) const noexcept -> BlockIndex {
    return BlockStringRangeView{*_data, _range}.indexOf(character, startIndex);
}

auto BlockStringEditor::indexOf(const CharSet &characterSet, const BlockIndex startIndex) const noexcept -> BlockIndex {
    return BlockStringRangeView{*_data, _range}.indexOf(characterSet, startIndex);
}

auto BlockStringEditor::indexNotOf(const CharSet &characterSet, const BlockIndex startIndex) const noexcept
    -> BlockIndex {
    return BlockStringRangeView{*_data, _range}.indexNotOf(characterSet, startIndex);
}

auto BlockStringEditor::slice(const BlockRange range) const noexcept -> BlockStringEditor {
    const auto storageRange = BlockStringRangeView{*_data, _range}.subRange(range);
    if (storageRange.isEmpty()) {
        return {};
    }
    return BlockStringEditor{_data, storageRange};
}

auto BlockStringEditor::slice(const StringSide side, const BlockCount count) const noexcept -> BlockStringEditor {
    if (side == StringSide::Back) {
        if (count >= length()) {
            return slice();
        }
        return slice(BlockRange{BlockIndex::end(length() - count), count});
    }
    return slice(BlockRange{BlockIndex::zero(), count});
}

auto BlockStringEditor::croppedToDisplayWidth(
    const bgeo::BlockCoordinate displayWidth, const bgeo::Alignment alignment) const noexcept -> BlockStringEditor {
    const auto range = BlockStringRangeView{*_data, _range}.croppedRange(displayWidth, alignment);
    if (range.isEmpty()) {
        return {};
    }
    return BlockStringEditor{_data, range};
}

auto BlockStringEditor::trimmed(const CharSet &characters) const noexcept -> BlockStringEditor {
    if (isEmpty()) {
        return {};
    }
    auto copy = *this;
    copy.trim(characters);
    return copy;
}

auto BlockStringEditor::normalized(const CharSet &characters, const Block separator) const noexcept
    -> BlockStringEditor {
    if (isEmpty()) {
        return {};
    }
    auto copy = *this;
    copy.normalize(characters, separator);
    return copy;
}

auto BlockStringEditor::containsControlCharacters() const noexcept -> bool {
    return BlockStringRangeView{*_data, _range}.containsControlCharacters();
}

void BlockStringEditor::reserve(const BlockCount size) noexcept {
    detach();
    _data->chars().reserve(size.toSizeT());
}

void BlockStringEditor::clear() noexcept {
    _data = impl::sharedEmptyBlockStringData();
    _range = {};
}

void BlockStringEditor::trim(const CharSet &characters) noexcept {
    if (isEmpty()) {
        return;
    }
    const auto range = BlockStringRangeView{*_data, _range}.trimmedRange(characters);
    if (range == _range) {
        return;
    }
    if (range.isEmpty()) {
        clear();
        return;
    }
    _range = range;
}

void BlockStringEditor::normalize(const CharSet &characters, const Block separator) noexcept {
    if (isEmpty()) {
        return;
    }
    trim(characters);
    auto startPos = indexOf(characters);
    while (!startPos.isNoIndex()) {
        auto endPos = indexNotOf(characters, startPos + BlockCount::one());
        if (endPos.isNoIndex()) {
            endPos = BlockIndex::end(length());
        }
        auto newSeparator = separator.withBase(at(startPos).style());
        const auto replacementLength = startPos.absoluteDistanceTo(endPos);
        if (!replacementLength.isOne() || at(startPos) != newSeparator) {
            replace(BlockRange{startPos, replacementLength}, newSeparator);
        }
        // Replacing the characters shortened the string.
        // Start the next search after the "not of" character previously found.
        startPos = indexOf(characters, startPos + BlockCount{2U});
    }
}

void BlockStringEditor::replace(BlockRange range, const Block replacement) noexcept {
    range = range.clampedTo(length());
    if (isEmpty() || range.isEmpty()) {
        return;
    }
    if (range.length().isOne() && !replacement.isEmpty()) { // fast path.
        if (this->at(range.index()) != replacement) {
            detach();
            (*this)[range.index()] = replacement;
        }
        return;
    }
    if (replacement.isEmpty()) { // empty char is like a remove operation.
        remove(range);
        return;
    }
    remove(BlockRange{range.index() + BlockCount::one(), range.length() - BlockCount::one()});
    if (at(range.index()) != replacement) {
        detach();
        _data->invalidateDisplayWidth();
        _data->chars()[range.index().toSizeT()] = replacement;
    }
}

void BlockStringEditor::replace(BlockRange range, const BlockString &replacement) noexcept {
    range = range.clampedTo(length());
    if (isEmpty() || range.isEmpty()) {
        return;
    }
    if (replacement.isEmpty()) {
        remove(range);
        return;
    }
    auto replacementChars = std::span(replacement.cbegin(), replacement.cend());
    const auto start = range.index().toSizeT();
    const auto rangeLength = range.length().toSizeT();
    const auto rangeEnd = range.endIndex().toSizeT();
    if (rangeLength == replacementChars.size()) { // fast path.
        auto firstMismatch = rangeLength;
        for (std::size_t i = 0; i < rangeLength; ++i) {
            if (at(BlockIndex::fromSizeT(start + i)) != replacementChars[i]) {
                firstMismatch = i;
                break;
            }
        }
        if (firstMismatch == rangeLength) {
            return;
        }
        detach();
        auto &chars = _data->chars();
        for (std::size_t i = firstMismatch; i < replacementChars.size(); ++i) {
            chars[start + i] = replacementChars[i];
        }
        _data->invalidateDisplayWidth();
        return;
    }
    detach();
    using Diff = BlockStringData::Storage::difference_type;
    auto &chars = _data->chars();
    if (rangeLength > replacementChars.size()) {
        chars.erase(
            chars.begin() + static_cast<Diff>(start + replacementChars.size()),
            chars.begin() + static_cast<Diff>(rangeEnd));
        for (std::size_t i = 0; i < replacementChars.size(); ++i) {
            chars[start + i] = replacementChars[i];
        }
    } else {
        const auto missingCharCount = replacementChars.size() - rangeLength;
        chars.insert(
            chars.begin() + static_cast<Diff>(start),
            replacementChars.begin(),
            replacementChars.begin() + static_cast<Diff>(missingCharCount));
        for (std::size_t i = missingCharCount; i < replacementChars.size(); ++i) {
            chars[start + i] = replacementChars[i];
        }
    }
    _data->invalidateDisplayWidth();
    syncRangeWithStorage();
}

void BlockStringEditor::remove(BlockRange range) noexcept {
    range = range.clampedTo(length());
    if (isEmpty() || range.isEmpty()) {
        return;
    }
    if (range.endIndex() >= BlockIndex::end(length())) { // fast path, string only needs trimming at the end.
        _range.setLength(BlockCount::fromSizeT(range.index().toSizeT()));
        return;
    }
    detach();
    using Diff = BlockStringData::Storage::difference_type;
    _data->chars().erase(
        _data->chars().begin() + static_cast<Diff>(range.index().toSizeT()),
        _data->chars().begin() + static_cast<Diff>(range.endIndex().toSizeT()));
    _data->invalidateDisplayWidth();
    syncRangeWithStorage();
}

void BlockStringEditor::set(const BlockIndex index, const Block character) noexcept {
    if (!index.isWithin(length())) {
        return;
    }
    if (const auto oldChar = at(index); oldChar != character) {
        detach();
        _data->chars()[index.toSizeT()] = character;
        if (oldChar.displayWidth() != character.displayWidth()) {
            _data->invalidateDisplayWidth();
        }
    }
}

void BlockStringEditor::insertWithBaseStyle(BlockIndex pos, const BlockString &other, const BlockStyle style) noexcept {
    if (other.isEmpty()) {
        return;
    }
    detach();
    if (!pos.isWithin(length())) {
        pos = BlockIndex::end(length());
    }
    _data->chars().insert(
        _data->chars().begin() + static_cast<std::ptrdiff_t>(pos.toSizeT()), other.length().toSizeT(), Block{});
    for (std::size_t i = 0; i < other.length().toSizeT(); ++i) {
        _data->chars()[pos.toSizeT() + i] = other[BlockIndex::fromSizeT(i)].withBase(style);
    }
    syncRangeWithStorage();
}

void BlockStringEditor::appendStyled(const String &text, const BlockStyle style) {
    detach();
    _data->appendCharacters(text, style.color(), style.attributes());
    syncRangeWithStorage();
}

void BlockStringEditor::appendStyled(const U32String &text, const BlockStyle style) noexcept {
    detach();
    _data->appendCharacters(text, style.color(), style.attributes());
    syncRangeWithStorage();
}

void BlockStringEditor::appendStyled(const BlockString &other, BlockStyle style) noexcept {
    appendStringWithBaseStyle(other, style);
}

void BlockStringEditor::append(const BlockCount count, const Block character) noexcept {
    if (count.isZero()) {
        return;
    }
    const auto limitedCount = std::min(count.toSizeT(), static_cast<std::size_t>(10'000'000U));
    detach();
    _data->chars().insert(_data->chars().end(), limitedCount, character);
    _data->invalidateDisplayWidth();
    syncRangeWithStorage();
}

void BlockStringEditor::append(const BlockCount count, const Char character, const BlockStyle style) noexcept {
    append(count, Block{character, style});
}

}
