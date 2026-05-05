// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BlockStringData.hpp"

#include "BlockTextUtil.hpp"

#include "../../text/StringCharReader.hpp"
#include "../../text/StringConverter.hpp"

namespace erbsland::cterm::impl {

BlockStringData::BlockStringData(Storage chars) noexcept : _chars{std::move(chars)} {
    for (const auto &character : _chars) {
        _displayWidth += character.displayWidth();
    }
}

BlockStringData::BlockStringData(Storage chars, const int displayWidth) noexcept :
    _chars{std::move(chars)}, _displayWidth{displayWidth} {
}

void BlockStringData::reserve(const std::size_t size) {
    _chars.reserve(size);
}

void BlockStringData::clear() noexcept {
    _chars.clear();
    _displayWidth = 0;
}

void BlockStringData::append(const Block &character) noexcept {
    _chars.emplace_back(character);
    addDisplayWidth(character.displayWidth());
}

void BlockStringData::addDisplayWidth(const int displayWidth) noexcept {
    if (hasDisplayWidthCache()) {
        _displayWidth += displayWidth;
    }
}

auto BlockStringData::copyChars(BlockRange range) const -> Storage {
    range = range.clampedTo(BlockCount::fromSizeT(_chars.size()));
    return Storage{
        _chars.cbegin() + static_cast<Storage::difference_type>(range.index().toSizeT()),
        _chars.cbegin() + static_cast<Storage::difference_type>(range.endIndex().toSizeT())};
}

void BlockStringData::appendCodePoint(const text::Char codePoint, const Color color, const BlockAttributes attributes) {
    if (codePoint == U'\t' || codePoint == U'\n') {
        append(Block{codePoint.toRawValue(), color, attributes});
        return;
    }
    if (isControlCode(codePoint)) {
        return;
    }
    if (text::impl::unicodeDisplayWidthFor(codePoint.toRawValue()) == 0) {
        if (!_chars.empty()) {
            _chars.back() = _chars.back().withCombining(codePoint);
        }
        return;
    }
    append(Block{codePoint.toRawValue(), color, attributes});
}

void BlockStringData::appendCharacters(
    const text::U32StringView &text, const Color color, const BlockAttributes attributes) noexcept {
    auto reader = text::StringCharReader{text};
    while (true) {
        const auto codePoint = reader.read();
        if (codePoint.isEndOfData()) {
            break;
        }
        appendCodePoint(codePoint, color, attributes);
    }
}

void BlockStringData::appendCharacters(
    const text::StringView &text,
    const Color color,
    const BlockAttributes attributes,
    const text::EncodingErrorMode encodingErrorMode) {
    appendCharacters(text::StringConverter{text}.toU32String(encodingErrorMode), color, attributes);
}

auto sharedEmptyBlockStringData() -> const BlockStringDataPtr & {
    static const auto data = BlockStringDataPtr{new BlockStringData{BlockStringData::Storage{}}};
    return data;
}

auto BlockStringData::measureDisplayWidth(const text::StringView &text, const text::EncodingErrorMode encodingErrorMode)
    -> int {
    return measureDisplayWidth(text::StringConverter{text}.toU32String(encodingErrorMode));
}

auto BlockStringData::measureDisplayWidth(const text::U32StringView &text) -> int {
    auto result = 0;
    auto reader = text::StringCharReader{text};
    while (true) {
        const auto codePoint = reader.read();
        if (codePoint.isEndOfData()) {
            break;
        }
        if (isStringCharacter(codePoint)) {
            result += Block{codePoint.toRawValue()}.displayWidth();
        }
    }
    return result;
}

}
