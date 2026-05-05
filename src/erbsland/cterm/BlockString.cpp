// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BlockString.hpp"

#include "BlockStringView.hpp"

#include "impl/BlockPrintContextToString.hpp"
#include "impl/BlockStringData.hpp"
#include "impl/BlockStringRangeView.hpp"
#include "impl/BlockStringWrapper.hpp"

#include <algorithm>
#include <span>
#include <utility>

namespace erbsland::cterm {

BlockString::BlockString() noexcept : _data{impl::sharedEmptyBlockStringData()} {
}

BlockString::BlockString(const text::StringView &str, const text::EncodingErrorMode encodingErrorMode) :
    BlockString{str, BlockStyle{}, encodingErrorMode} {
}

BlockString::BlockString(
    const text::StringView &str, const BlockStyle style, const text::EncodingErrorMode encodingErrorMode) :
    BlockString{splitCharacters(str, style.color(), style.attributes(), encodingErrorMode)} {
}

BlockString::BlockString(const text::U32StringView &str) : BlockString{str, BlockStyle{}} {
}

BlockString::BlockString(const text::U32StringView &str, const BlockStyle style) :
    BlockString{splitCharacters(str, style.color(), style.attributes())} {
}

BlockString::BlockString(const BlockCount count, const Block character) noexcept :
    BlockString{Storage{std::min(count.toSizeT(), static_cast<std::size_t>(10'000'000U)), character}} {
}

BlockString::BlockString(const BlockStringView &view) : BlockString{view._data->copyChars(view._range)} {
}

BlockString::BlockString(impl::BlockStringDataPtr data, const BlockRange range) noexcept :
    _data{std::move(data)}, _range{range.clampedTo(BlockCount::fromSizeT(_data->size()))} {
}

BlockString::BlockString(Storage chars) noexcept :
    _data{
        chars.empty() ? impl::sharedEmptyBlockStringData()
                      : impl::BlockStringDataPtr{new impl::BlockStringData{std::move(chars)}}},
    _range{BlockIndex::zero(), BlockCount::fromSizeT(_data->size())} {
}

auto BlockString::operator==(const BlockString &other) const noexcept -> bool {
    const auto view = impl::BlockStringRangeView{*_data, _range};
    const auto otherView = impl::BlockStringRangeView{*other._data, other._range};
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

auto BlockString::operator[](const BlockIndex index) const noexcept -> Block {
    return impl::BlockStringRangeView{*_data, _range}[index];
}

auto BlockString::operator[](const BlockIndex index) noexcept -> Block & {
    if (!index.isWithin(length())) {
        return ignoredMutableCharacter();
    }
    detach();
    _data->invalidateDisplayWidth();
    return _data->chars()[index.toSizeT()];
}

auto BlockString::operator+=(const BlockString &other) noexcept -> BlockString & {
    append(other);
    return *this;
}

auto BlockString::operator+=(const BlockStringView &other) noexcept -> BlockString & {
    append(other);
    return *this;
}

auto BlockString::operator+(const BlockString &other) const noexcept -> BlockString {
    auto copy = *this;
    copy.append(other);
    return copy;
}

auto BlockString::operator+(const BlockStringView &other) const noexcept -> BlockString {
    auto copy = *this;
    copy.append(other);
    return copy;
}

auto BlockString::displayWidth() const noexcept -> int {
    return impl::BlockStringRangeView{*_data, _range}.displayWidth();
}

auto BlockString::at(const BlockIndex index) const -> Block {
    return impl::BlockStringRangeView{*_data, _range}.at(index, "BlockString");
}

auto BlockString::begin() noexcept -> iterator {
    detach();
    _data->invalidateDisplayWidth();
    return _data->chars().begin();
}

auto BlockString::end() noexcept -> iterator {
    detach();
    _data->invalidateDisplayWidth();
    return _data->chars().end();
}

auto BlockString::begin() const noexcept -> const_iterator {
    return impl::BlockStringRangeView{*_data, _range}.begin();
}

auto BlockString::end() const noexcept -> const_iterator {
    return impl::BlockStringRangeView{*_data, _range}.end();
}

auto BlockString::cbegin() const noexcept -> const_iterator {
    return impl::BlockStringRangeView{*_data, _range}.cbegin();
}

auto BlockString::cend() const noexcept -> const_iterator {
    return impl::BlockStringRangeView{*_data, _range}.cend();
}

auto BlockString::rbegin() noexcept -> reverse_iterator {
    return reverse_iterator{end()};
}

auto BlockString::rend() noexcept -> reverse_iterator {
    return reverse_iterator{begin()};
}

auto BlockString::rbegin() const noexcept -> const_reverse_iterator {
    return impl::BlockStringRangeView{*_data, _range}.rbegin();
}

auto BlockString::rend() const noexcept -> const_reverse_iterator {
    return impl::BlockStringRangeView{*_data, _range}.rend();
}

auto BlockString::crbegin() const noexcept -> const_reverse_iterator {
    return impl::BlockStringRangeView{*_data, _range}.crbegin();
}

auto BlockString::crend() const noexcept -> const_reverse_iterator {
    return impl::BlockStringRangeView{*_data, _range}.crend();
}

auto BlockString::count(const Block &character) const noexcept -> BlockCount {
    return impl::BlockStringRangeView{*_data, _range}.count(character);
}

auto BlockString::count(const text::Char character) const noexcept -> BlockCount {
    return impl::BlockStringRangeView{*_data, _range}.count(character);
}

auto BlockString::indexOf(const Block &character, const BlockIndex startIndex) const noexcept -> BlockIndex {
    return impl::BlockStringRangeView{*_data, _range}.indexOf(character, startIndex);
}

auto BlockString::indexOf(const text::Char character, const BlockIndex startIndex) const noexcept -> BlockIndex {
    return impl::BlockStringRangeView{*_data, _range}.indexOf(character, startIndex);
}

auto BlockString::indexOf(const text::CharSet &characterSet, const BlockIndex startIndex) const noexcept -> BlockIndex {
    return impl::BlockStringRangeView{*_data, _range}.indexOf(characterSet, startIndex);
}

auto BlockString::indexNotOf(const text::CharSet &characterSet, const BlockIndex startIndex) const noexcept
    -> BlockIndex {
    return impl::BlockStringRangeView{*_data, _range}.indexNotOf(characterSet, startIndex);
}

auto BlockString::slice(const BlockRange range) const noexcept -> BlockString {
    const auto storageRange = impl::BlockStringRangeView{*_data, _range}.subRange(range);
    if (storageRange.isEmpty()) {
        return {};
    }
    return BlockString{_data, storageRange};
}

auto BlockString::slice(const text::StringSide side, const BlockCount count) const noexcept -> BlockString {
    if (side == text::StringSide::Back) {
        if (count >= length()) {
            return slice();
        }
        return slice(BlockRange{BlockIndex::end(length() - count), count});
    }
    return slice(BlockRange{BlockIndex::zero(), count});
}

auto BlockString::croppedToDisplayWidth(
    const bgeo::BlockCoordinate displayWidth, const bgeo::Alignment alignment) const noexcept -> BlockString {
    const auto range = impl::BlockStringRangeView{*_data, _range}.croppedRange(displayWidth, alignment);
    if (range.isEmpty()) {
        return {};
    }
    return BlockString{_data, range};
}

auto BlockString::trimmed(const text::CharSet &characters) const noexcept -> BlockString {
    if (isEmpty()) {
        return {};
    }
    auto copy = *this;
    copy.trim(characters);
    return copy;
}

auto BlockString::normalized(const text::CharSet &characters, const Block separator) const noexcept -> BlockString {
    if (isEmpty()) {
        return {};
    }
    auto copy = *this;
    copy.normalize(characters, separator);
    return copy;
}

auto BlockString::containsControlCharacters() const noexcept -> bool {
    return impl::BlockStringRangeView{*_data, _range}.containsControlCharacters();
}

void BlockString::reserve(const BlockCount size) noexcept {
    detach();
    _data->chars().reserve(size.toSizeT());
}

void BlockString::clear() noexcept {
    _data = impl::sharedEmptyBlockStringData();
    _range = {};
}

void BlockString::trim(const text::CharSet &characters) noexcept {
    if (isEmpty()) {
        return;
    }
    const auto range = impl::BlockStringRangeView{*_data, _range}.trimmedRange(characters);
    if (range == _range) {
        return;
    }
    if (range.isEmpty()) {
        clear();
        return;
    }
    _range = range;
}

void BlockString::normalize(const text::CharSet &characters, const Block separator) noexcept {
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

void BlockString::replace(BlockRange range, const Block replacement) noexcept {
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

void BlockString::replace(BlockRange range, const BlockStringView &replacement) noexcept {
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
    using Diff = impl::BlockStringData::Storage::difference_type;
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

void BlockString::remove(BlockRange range) noexcept {
    range = range.clampedTo(length());
    if (isEmpty() || range.isEmpty()) {
        return;
    }
    if (range.endIndex() >= BlockIndex::end(length())) { // fast path, string only needs trimming at the end.
        _range.setLength(BlockCount::fromSizeT(range.index().toSizeT()));
        return;
    }
    detach();
    using Diff = impl::BlockStringData::Storage::difference_type;
    _data->chars().erase(
        _data->chars().begin() + static_cast<Diff>(range.index().toSizeT()),
        _data->chars().begin() + static_cast<Diff>(range.endIndex().toSizeT()));
    _data->invalidateDisplayWidth();
    syncRangeWithStorage();
}

void BlockString::set(const BlockIndex index, const Block character) noexcept {
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

void BlockString::insertWithBaseStyle(BlockIndex pos, const BlockStringView &other, const BlockStyle style) noexcept {
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

void BlockString::appendStyled(
    const text::StringView &text, const BlockStyle style, const text::EncodingErrorMode encodingErrorMode) {
    detach();
    _data->appendCharacters(text, style.color(), style.attributes(), encodingErrorMode);
    syncRangeWithStorage();
}

void BlockString::appendStyled(const text::U32StringView &text, const BlockStyle style) noexcept {
    detach();
    _data->appendCharacters(text, style.color(), style.attributes());
    syncRangeWithStorage();
}

void BlockString::appendStyled(const BlockStringView &other, BlockStyle style) noexcept {
    appendViewWithBaseStyle(other, style);
}

void BlockString::append(const BlockCount count, const Block character) noexcept {
    if (count.isZero()) {
        return;
    }
    const auto limitedCount = std::min(count.toSizeT(), static_cast<std::size_t>(10'000'000U));
    detach();
    _data->chars().insert(_data->chars().end(), limitedCount, character);
    _data->invalidateDisplayWidth();
    syncRangeWithStorage();
}

void BlockString::append(const BlockCount count, const text::Char character, const BlockStyle style) noexcept {
    append(count, Block{character, style});
}

auto BlockString::splitWords() const noexcept -> std::vector<BlockString> {
    auto words = std::vector<BlockString>{};
    for (const auto &word : BlockStringView{*this}.splitWords()) {
        words.push_back(BlockString{word._data, word._range});
    }
    return words;
}

auto BlockString::wrapIntoLines(const int width, const ParagraphSpacing paragraphSpacing) const noexcept
    -> std::vector<BlockString> {
    return impl::BlockStringWrapper{*this}.wrapIntoLines(width, paragraphSpacing);
}

auto BlockString::terminalLines(const int width) const noexcept -> int {
    return BlockStringView{*this}.terminalLines(width);
}

auto BlockString::naturalBlockTextSize() const noexcept -> bgeo::BlockSize {
    return BlockStringView{*this}.naturalBlockTextSize();
}

auto BlockString::wrappedBlockTextHeight(
    const bgeo::BlockCoordinate width, const BlockTextOptions &options) const noexcept -> bgeo::BlockCoordinate {
    return BlockStringView{*this}.wrappedBlockTextHeight(width, options);
}

auto BlockString::splitLines() const noexcept -> std::vector<BlockString> {
    auto result = std::vector<BlockString>{};
    for (const auto &line : BlockStringView{*this}.splitLines()) {
        result.push_back(BlockString{line._data, line._range});
    }
    return result;
}

auto BlockString::withBase(const BlockStyle style) const noexcept -> BlockString {
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

auto BlockString::fromLines(
    const std::initializer_list<text::StringView> lines, const Color color, const BlockAttributes attributes) noexcept
    -> BlockString {
    if (lines.size() == 0) {
        return {};
    }
    auto data = impl::BlockStringData{};
    auto reservedCapacity = lines.size() - 1U; // count newlines
    for (auto &line : lines) {
        reservedCapacity += static_cast<std::size_t>(impl::BlockStringData::measureDisplayWidth(line));
    }
    data.reserve(reservedCapacity);
    bool isFirstLine = true;
    for (const auto &line : lines) {
        if (isFirstLine) {
            isFirstLine = false;
        } else {
            data.append(Block{U'\n', color, attributes});
        }
        data.appendCharacters(line, color, attributes, text::EncodingErrorMode::Replace);
    }
    return fromStorageWithDisplayWidth(std::move(data.chars()), data.displayWidth());
}

auto BlockString::fromLines(const std::initializer_list<text::StringView> lines, const BlockStyle style) noexcept
    -> BlockString {
    return fromLines(lines, style.color(), style.attributes());
}

auto BlockString::fromLines(
    const std::initializer_list<text::U32StringView> lines,
    const Color color,
    const BlockAttributes attributes) noexcept -> BlockString {
    if (lines.size() == 0) {
        return {};
    }
    auto data = impl::BlockStringData{};
    auto reservedCapacity = lines.size() - 1U; // count newlines
    for (auto &line : lines) {
        reservedCapacity += static_cast<std::size_t>(impl::BlockStringData::measureDisplayWidth(line));
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

auto BlockString::fromLines(const std::initializer_list<text::U32StringView> lines, const BlockStyle style) noexcept
    -> BlockString {
    return fromLines(lines, style.color(), style.attributes());
}

auto BlockString::splitCharacters(
    const text::StringView &str,
    const Color color,
    const BlockAttributes attributes,
    const text::EncodingErrorMode encodingErrorMode) -> Storage {
    auto data = impl::BlockStringData{};
    data.reserve(static_cast<std::size_t>(impl::BlockStringData::measureDisplayWidth(str, encodingErrorMode)));
    data.appendCharacters(str, color, attributes, encodingErrorMode);
    return std::move(data.chars());
}

auto BlockString::splitCharacters(const text::U32StringView &str, const Color color, const BlockAttributes attributes)
    -> Storage {
    auto data = impl::BlockStringData{};
    data.reserve(str.length().toSizeT());
    data.appendCharacters(str, color, attributes);
    return std::move(data.chars());
}

auto BlockString::createPrintContext() noexcept -> BlockPrintContextPtr {
    return std::make_unique<impl::BlockPrintContextToBlockString>(*this);
}

void BlockString::appendView(const BlockStringView &view, const BlockStyle style) noexcept {
    static_cast<void>(style);
    if (view.isEmpty()) {
        return;
    }
    detach();
    auto &chars = _data->chars();
    chars.insert(chars.end(), view.begin(), view.end());
    _data->addDisplayWidth(view.displayWidth());
    syncRangeWithStorage();
}

void BlockString::appendViewWithBaseStyle(const BlockStringView &view, const BlockStyle style) noexcept {
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

void BlockString::detach() {
    if (_range.index().isZero() && _range.length().toSizeT() == _data->size() && _data.useCount() == 1) {
        return;
    }
    _data = impl::BlockStringDataPtr{new impl::BlockStringData{_data->copyChars(_range)}};
    _range = BlockRange{BlockIndex::zero(), BlockCount::fromSizeT(_data->size())};
}

auto BlockString::fromStorageWithDisplayWidth(Storage chars, const int displayWidth) noexcept -> BlockString {
    if (chars.empty()) {
        return {};
    }
    const auto size = chars.size();
    auto data = impl::BlockStringDataPtr{new impl::BlockStringData{std::move(chars), displayWidth}};
    return BlockString{std::move(data), BlockRange{BlockIndex::zero(), BlockCount::fromSizeT(size)}};
}

void BlockString::syncRangeWithStorage() noexcept {
    _range = BlockRange{BlockIndex::zero(), BlockCount::fromSizeT(_data->size())};
}

auto BlockString::characterAt(const BlockIndex index) const noexcept -> const Block & {
    return impl::BlockStringRangeView{*_data, _range}.characterAt(index);
}

auto BlockString::defaultTrimCharacters() -> const text::CharSet & {
    static const auto characters = text::CharSet{{U' ', U'\n', U'\t'}};
    return characters;
}

auto BlockString::ignoredMutableCharacter() noexcept -> Block & {
    static thread_local auto ignored = Block{};
    ignored = Block{};
    return ignored;
}

}
