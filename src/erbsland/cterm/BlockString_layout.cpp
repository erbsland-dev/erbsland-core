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
