// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CombinedChar.hpp"

#include "EncodingError.hpp"
#include "StringCharReader.hpp"

namespace erbsland::text {

CombinedChar::CombinedChar(const String &text) noexcept : _characters{decodeUtf8(text)} {
}

CombinedChar::CombinedChar(const U32String &text) noexcept : _characters{decodeUtf32(text)} {
}

auto CombinedChar::toString() const -> String {
    auto result = StringEditor{};
    for (const auto codePoint : _characters) {
        if (codePoint.isNull()) {
            break;
        }
        result.append(codePoint);
    }
    return result;
}

auto CombinedChar::toU32String() const -> U32String {
    auto result = U32StringEditor{};
    for (const auto codePoint : _characters) {
        if (codePoint.isNull()) {
            break;
        }
        result.append(codePoint);
    }
    return result;
}

auto CombinedChar::byteCount() const noexcept -> unit::ByteLength {
    auto result = unit::ByteLength::zero();
    for (const auto codePoint : _characters) {
        if (codePoint.isNull()) {
            break;
        }
        result += codePoint.utf8Size();
    }
    return result;
}

auto CombinedChar::withCombining(const Char codePoint, const EncodingErrorMode encodingErrors) const -> CombinedChar {
    if (characterCount().isZero()) {
        if (encodingErrors == EncodingErrorMode::Throw) {
            throw EncodingError{"A base code point is required before combining code points can be appended."};
        }
        return *this;
    }
    if (!codePoint.isValidUnicode()) {
        if (encodingErrors == EncodingErrorMode::Throw) {
            throw EncodingError{"CombinedChar requires a valid Unicode code point."};
        }
        return *this;
    }
    if (codePoint.isNull()) {
        if (encodingErrors == EncodingErrorMode::Throw) {
            throw EncodingError{"CombinedChar does not support the Unicode code point U+0000."};
        }
        return *this;
    }
    if (codePoint.isControl()) {
        if (encodingErrors == EncodingErrorMode::Throw) {
            throw EncodingError{"CombinedChar combining code points must not be control codes."};
        }
        return *this;
    }
    if (codePoint.displayWidth() != 0) {
        if (encodingErrors == EncodingErrorMode::Throw) {
            throw EncodingError{"CombinedChar combining code points must have zero display width."};
        }
        return *this;
    }
    auto result = *this;
    const auto index = countCodePoints(result._characters);
    if (index >= result._characters.size()) {
        if (encodingErrors == EncodingErrorMode::Throw) {
            throw EncodingError{"CombinedChar supports at most three Unicode code points."};
        }
        return *this;
    }
    result._characters[index] = codePoint;
    return result;
}

auto CombinedChar::fromString(const String &text) noexcept -> CombinedChar {
    auto result = CombinedChar{};
    result._characters = decodeUtf8(text);
    return result;
}

auto CombinedChar::fromString(const U32String &text) noexcept -> CombinedChar {
    auto result = CombinedChar{};
    result._characters = decodeUtf32(text);
    return result;
}

auto CombinedChar::decodeUtf8(const String &text) noexcept -> Storage {
    auto result = Storage{};
    auto combiningCount = std::size_t{0};
    auto hasBaseCodePoint = false;
    auto mustReplace = false;
    auto reader = StringCharReader{text};
    while (!reader.isAtEnd()) {
        normalizeDecodedTextCodePoint(result, combiningCount, hasBaseCodePoint, mustReplace, reader.read());
    }
    if (mustReplace || !hasBaseCodePoint) {
        return replacementStorage();
    }
    return result;
}

auto CombinedChar::decodeUtf32(const U32String &text) noexcept -> Storage {
    return normalizeDecodedText(text);
}

auto CombinedChar::normalizeTextCodePoint(const Char codePoint) noexcept -> Char {
    if (!codePoint.isValidUnicode()) {
        return Char{U'\uFFFD'};
    }
    return codePoint;
}

auto CombinedChar::normalizeDecodedText(const U32String &text) noexcept -> Storage {
    auto result = Storage{};
    auto combiningCount = std::size_t{0};
    auto hasBaseCodePoint = false;
    auto mustReplace = false;
    auto reader = StringCharReader{text};
    while (!reader.isAtEnd()) {
        normalizeDecodedTextCodePoint(
            result, combiningCount, hasBaseCodePoint, mustReplace, normalizeTextCodePoint(reader.read()));
    }
    if (mustReplace || !hasBaseCodePoint) {
        return replacementStorage();
    }
    return result;
}

auto CombinedChar::replacementStorage() noexcept -> Storage {
    return {Char{U'\uFFFD'}, {}, {}};
}

void CombinedChar::normalizeDecodedTextCodePoint(
    Storage &result,
    std::size_t &combiningCount,
    bool &hasBaseCodePoint,
    bool &mustReplace,
    const Char codePoint) noexcept {
    if (mustReplace) {
        return;
    }
    if (!hasBaseCodePoint) {
        if (codePoint.isNull() || codePoint.isControl() || codePoint.displayWidth() == 0) {
            mustReplace = true;
            return;
        }
        result[0] = codePoint;
        hasBaseCodePoint = true;
        return;
    }
    if (codePoint.isNull() || codePoint.isControl()) {
        mustReplace = true;
        return;
    }
    if (codePoint.displayWidth() != 0) {
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
