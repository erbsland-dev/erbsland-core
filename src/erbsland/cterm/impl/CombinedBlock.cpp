// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CombinedBlock.hpp"

#include "../../text/StringCharReader.hpp"
#include "../../text/StringConverter.hpp"

#include <stdexcept>

namespace erbsland::cterm::impl {

CombinedBlock::CombinedBlock(const text::StringView &text) noexcept : _codePoints{decodeUtf8(text)} {
}

CombinedBlock::CombinedBlock(const text::U32StringView &text) noexcept : _codePoints{decodeUtf32(text)} {
}

auto CombinedBlock::utf8() const -> text::String {
    auto result = text::String{};
    for (const auto codePoint : _codePoints) {
        if (codePoint.isNull()) {
            break;
        }
        result.append(codePoint);
    }
    return result;
}

auto CombinedBlock::utf32() const -> text::U32String {
    auto result = text::U32String{};
    for (const auto codePoint : _codePoints) {
        if (codePoint.isNull()) {
            break;
        }
        result.append(codePoint);
    }
    return result;
}

auto CombinedBlock::byteCount() const noexcept -> std::size_t {
    auto result = std::size_t{0};
    for (const auto codePoint : _codePoints) {
        if (codePoint.isNull()) {
            break;
        }
        result += codePoint.utf8Size().toSizeT();
    }
    return result;
}

void CombinedBlock::appendTo(std::string &buffer) const noexcept {
    const auto text = utf8();
    const auto encodedText = text::StringConverter{text}.toStdString();
    buffer.reserve(buffer.size() + encodedText.size());
    buffer.append(encodedText);
}

auto CombinedBlock::withCombining(const text::Char codePoint, const text::EncodingErrorMode encodingErrors) const
    -> CombinedBlock {
    if (codePointCount() == 0) {
        if (encodingErrors == text::EncodingErrorMode::Throw) {
            throw std::invalid_argument{"A base code point is required before combining code points can be appended."};
        }
        return *this;
    }
    if (!codePoint.isValidUnicode()) {
        if (encodingErrors == text::EncodingErrorMode::Throw) {
            throw std::invalid_argument{"CombinedBlock requires a valid Unicode code point."};
        }
        return *this;
    }
    if (codePoint.isNull()) {
        if (encodingErrors == text::EncodingErrorMode::Throw) {
            throw std::invalid_argument{"CombinedBlock does not support the Unicode code point U+0000."};
        }
        return *this;
    }
    if (isControlCode(codePoint)) {
        if (encodingErrors == text::EncodingErrorMode::Throw) {
            throw std::invalid_argument{"CombinedBlock combining code points must not be control codes."};
        }
        return *this;
    }
    if (text::impl::unicodeDisplayWidthFor(codePoint.toRawValue()) != 0) {
        if (encodingErrors == text::EncodingErrorMode::Throw) {
            throw std::invalid_argument{"CombinedBlock combining code points must have zero display width."};
        }
        return *this;
    }
    auto result = *this;
    const auto index = countCodePoints(result._codePoints);
    if (index >= result._codePoints.size()) {
        if (encodingErrors == text::EncodingErrorMode::Throw) {
            throw std::invalid_argument{"CombinedBlock supports at most three Unicode code points."};
        }
        return *this;
    }
    result._codePoints[index] = codePoint;
    return result;
}

auto CombinedBlock::fromTextUtf8(const text::StringView &text) noexcept -> std::optional<CombinedBlock> {
    auto result = CombinedBlock{};
    result._codePoints = decodeUtf8(text);
    return result;
}

auto CombinedBlock::fromTextUtf32(const text::U32StringView &text) noexcept -> std::optional<CombinedBlock> {
    auto result = CombinedBlock{};
    result._codePoints = decodeUtf32(text);
    return result;
}

auto CombinedBlock::isTextCodePoint(const text::Char codePoint) noexcept -> bool {
    return codePoint == U'\t' || codePoint == U'\n' ||
        (!isControlCode(codePoint) && codePoint >= U' ' &&
            text::impl::unicodeDisplayWidthFor(codePoint.toRawValue()) > 0);
}

auto CombinedBlock::decodeUtf8(const text::StringView &text) noexcept -> Storage {
    auto result = Storage{};
    auto combiningCount = std::size_t{0};
    auto hasBaseCodePoint = false;
    auto mustReplace = false;
    auto reader = text::StringCharReader{text};
    while (!reader.isAtEnd()) {
        normalizeDecodedTextCodePoint(result, combiningCount, hasBaseCodePoint, mustReplace, reader.read());
    }
    if (mustReplace || !hasBaseCodePoint) {
        return replacementStorage();
    }
    return result;
}

auto CombinedBlock::decodeUtf32(const text::U32StringView &text) noexcept -> Storage {
    return normalizeDecodedText(text);
}

auto CombinedBlock::normalizeTextCodePoint(const text::Char codePoint) noexcept -> text::Char {
    if (!codePoint.isValidUnicode()) {
        return text::Char{U'\uFFFD'};
    }
    return codePoint;
}

auto CombinedBlock::normalizeDecodedText(const text::U32StringView &text) noexcept -> Storage {
    auto result = Storage{};
    auto combiningCount = std::size_t{0};
    auto hasBaseCodePoint = false;
    auto mustReplace = false;
    auto reader = text::StringCharReader{text};
    while (!reader.isAtEnd()) {
        normalizeDecodedTextCodePoint(
            result, combiningCount, hasBaseCodePoint, mustReplace, normalizeTextCodePoint(reader.read()));
    }
    if (mustReplace || !hasBaseCodePoint) {
        return replacementStorage();
    }
    return result;
}

auto CombinedBlock::replacementStorage() noexcept -> Storage {
    return {text::Char{U'\uFFFD'}, {}, {}};
}

void CombinedBlock::normalizeDecodedTextCodePoint(
    Storage &result,
    std::size_t &combiningCount,
    bool &hasBaseCodePoint,
    bool &mustReplace,
    const text::Char codePoint) noexcept {
    if (mustReplace) {
        return;
    }
    if (!hasBaseCodePoint) {
        if (codePoint.isNull() || isControlCode(codePoint) ||
            text::impl::unicodeDisplayWidthFor(codePoint.toRawValue()) == 0) {
            mustReplace = true;
            return;
        }
        result[0] = codePoint;
        hasBaseCodePoint = true;
        return;
    }
    if (codePoint.isNull() || isControlCode(codePoint)) {
        mustReplace = true;
        return;
    }
    if (text::impl::unicodeDisplayWidthFor(codePoint.toRawValue()) != 0) {
        mustReplace = true;
        return;
    }
    if (combiningCount >= 2) {
        return;
    }
    result[1 + combiningCount] = codePoint;
    ++combiningCount;
}

}
