// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TextRingBuffer.hpp"

#include "StringCharReader.hpp"
#include "StringEncoder.hpp"

#include "../err/OverflowError.hpp"
#include "../mem/ByteBlockView.hpp"

#include <array>
#include <cstddef>
#include <limits>
#include <span>

namespace erbsland::text {

namespace {

auto effectiveEncoding(const StringEncoding encoding) noexcept -> StringEncoding {
    switch (encoding) {
    case StringEncoding::Utf16:
        return StringEncoding::Utf16LittleEndian;
    case StringEncoding::Utf32:
        return StringEncoding::Utf32LittleEndian;
    default:
        return encoding;
    }
}

auto hasBom(const StringEncoding encoding, const StringBomMode mode) noexcept -> bool {
    if (mode == StringBomMode::Reject) {
        return false;
    }
    return mode == StringBomMode::Require || encoding != StringEncoding::Utf8;
}

auto bomLength(const StringEncoding encoding, const StringBomMode mode) noexcept -> std::size_t {
    if (!hasBom(encoding, mode)) {
        return 0U;
    }
    switch (effectiveEncoding(encoding)) {
    case StringEncoding::Utf8:
        return 3U;
    case StringEncoding::Utf16LittleEndian:
    case StringEncoding::Utf16BigEndian:
        return 2U;
    case StringEncoding::Utf32LittleEndian:
    case StringEncoding::Utf32BigEndian:
        return 4U;
    case StringEncoding::Utf16:
    case StringEncoding::Utf32:
        break;
    }
    return 0U;
}

auto characterLength(const Char character, const StringEncoding encoding) noexcept -> std::size_t {
    const auto codePoint = character.toRawValue();
    switch (effectiveEncoding(encoding)) {
    case StringEncoding::Utf8:
        if (codePoint <= 0x7fU) {
            return 1U;
        }
        if (codePoint <= 0x7ffU) {
            return 2U;
        }
        if (codePoint <= 0xffffU) {
            return 3U;
        }
        return 4U;
    case StringEncoding::Utf16LittleEndian:
    case StringEncoding::Utf16BigEndian:
        return codePoint <= 0xffffU ? 2U : 4U;
    case StringEncoding::Utf32LittleEndian:
    case StringEncoding::Utf32BigEndian:
        return 4U;
    case StringEncoding::Utf16:
    case StringEncoding::Utf32:
        break;
    }
    return 0U;
}

auto bomBytes(const StringEncoding encoding, const StringBomMode mode) -> std::span<const mem::Byte> {
    static constexpr auto cUtf8 = std::array{mem::Byte{0xefU}, mem::Byte{0xbbU}, mem::Byte{0xbfU}};
    static constexpr auto cUtf16Le = std::array{mem::Byte{0xffU}, mem::Byte{0xfeU}};
    static constexpr auto cUtf16Be = std::array{mem::Byte{0xfeU}, mem::Byte{0xffU}};
    static constexpr auto cUtf32Le = std::array{mem::Byte{0xffU}, mem::Byte{0xfeU}, mem::Byte{0x00U}, mem::Byte{0x00U}};
    static constexpr auto cUtf32Be = std::array{mem::Byte{0x00U}, mem::Byte{0x00U}, mem::Byte{0xfeU}, mem::Byte{0xffU}};
    if (!hasBom(encoding, mode)) {
        return {};
    }
    switch (effectiveEncoding(encoding)) {
    case StringEncoding::Utf8:
        return cUtf8;
    case StringEncoding::Utf16LittleEndian:
        return cUtf16Le;
    case StringEncoding::Utf16BigEndian:
        return cUtf16Be;
    case StringEncoding::Utf32LittleEndian:
        return cUtf32Le;
    case StringEncoding::Utf32BigEndian:
        return cUtf32Be;
    case StringEncoding::Utf16:
    case StringEncoding::Utf32:
        break;
    }
    return {};
}

auto encodeCharacter(const Char character, const StringEncoding encoding, std::array<mem::Byte, 4> &bytes)
    -> std::size_t {
    const auto value = static_cast<uint32_t>(character.toRawValue());
    switch (effectiveEncoding(encoding)) {
    case StringEncoding::Utf8:
        if (value <= 0x7fU) {
            bytes[0] = mem::Byte{static_cast<uint8_t>(value)};
            return 1U;
        }
        if (value <= 0x7ffU) {
            bytes[0] = mem::Byte{static_cast<uint8_t>(0xc0U | (value >> 6U))};
            bytes[1] = mem::Byte{static_cast<uint8_t>(0x80U | (value & 0x3fU))};
            return 2U;
        }
        if (value <= 0xffffU) {
            bytes[0] = mem::Byte{static_cast<uint8_t>(0xe0U | (value >> 12U))};
            bytes[1] = mem::Byte{static_cast<uint8_t>(0x80U | ((value >> 6U) & 0x3fU))};
            bytes[2] = mem::Byte{static_cast<uint8_t>(0x80U | (value & 0x3fU))};
            return 3U;
        }
        bytes[0] = mem::Byte{static_cast<uint8_t>(0xf0U | (value >> 18U))};
        bytes[1] = mem::Byte{static_cast<uint8_t>(0x80U | ((value >> 12U) & 0x3fU))};
        bytes[2] = mem::Byte{static_cast<uint8_t>(0x80U | ((value >> 6U) & 0x3fU))};
        bytes[3] = mem::Byte{static_cast<uint8_t>(0x80U | (value & 0x3fU))};
        return 4U;
    case StringEncoding::Utf16LittleEndian:
    case StringEncoding::Utf16BigEndian: {
        const auto bigEndian = effectiveEncoding(encoding) == StringEncoding::Utf16BigEndian;
        const auto writeUnit = [&bytes, bigEndian](const uint16_t unit, const std::size_t offset) {
            bytes[offset + (bigEndian ? 1U : 0U)] = mem::Byte{static_cast<uint8_t>(unit & 0xffU)};
            bytes[offset + (bigEndian ? 0U : 1U)] = mem::Byte{static_cast<uint8_t>(unit >> 8U)};
        };
        if (value <= 0xffffU) {
            writeUnit(static_cast<uint16_t>(value), 0U);
            return 2U;
        }
        const auto adjusted = value - 0x10000U;
        writeUnit(static_cast<uint16_t>(0xd800U | (adjusted >> 10U)), 0U);
        writeUnit(static_cast<uint16_t>(0xdc00U | (adjusted & 0x3ffU)), 2U);
        return 4U;
    }
    case StringEncoding::Utf32LittleEndian:
    case StringEncoding::Utf32BigEndian: {
        const auto bigEndian = effectiveEncoding(encoding) == StringEncoding::Utf32BigEndian;
        for (auto i = std::size_t{0}; i < 4U; ++i) {
            const auto target = bigEndian ? 3U - i : i;
            bytes[target] = mem::Byte{static_cast<uint8_t>((value >> (i * 8U)) & 0xffU)};
        }
        return 4U;
    }
    case StringEncoding::Utf16:
    case StringEncoding::Utf32:
        break;
    }
    return 0U;
}

}

auto TextRingBuffer::encodedLength(
    const StringView &text,
    const StringEncoding encoding,
    const StringBomMode bomMode,
    const EncodingErrorMode errorMode) -> unit::ByteLength {
    if (errorMode != EncodingErrorMode::Replace) {
        return StringEncoder{text}.encode(encoding, bomMode, errorMode).length();
    }
    auto length = bomLength(encoding, bomMode);
    auto reader = StringCharReader{text};
    while (true) {
        const auto character = reader.read();
        if (character.isEndOfData()) {
            break;
        }
        const auto additionalLength = characterLength(character, encoding);
        if (additionalLength > std::numeric_limits<std::size_t>::max() - length) {
            throw err::OverflowError{"The encoded text exceeds the supported byte length."};
        }
        length += additionalLength;
    }
    return unit::ByteLength::fromSizeT(length);
}

auto TextRingBuffer::writeEncoded(
    const StringView &text,
    const StringEncoding encoding,
    const StringBomMode bomMode,
    const EncodingErrorMode errorMode) -> bool {
    if (errorMode != EncodingErrorMode::Replace) {
        const auto encoded = StringEncoder{text}.encode(encoding, bomMode, errorMode);
        return isSuccessful(writeExact(mem::ByteBlockView{encoded}.bytes()));
    }
    const auto length = encodedLength(text, encoding, bomMode, errorMode);
    if (isFailure(reserveAdditional(length))) {
        return false;
    }
    const auto bom = bomBytes(encoding, bomMode);
    static_cast<void>(writeExact(bom));
    auto reader = StringCharReader{text};
    auto bytes = std::array<mem::Byte, 4>{};
    while (true) {
        const auto character = reader.read();
        if (character.isEndOfData()) {
            break;
        }
        const auto count = encodeCharacter(character, encoding, bytes);
        static_cast<void>(writeExact(std::span<const mem::Byte>{bytes.data(), count}));
    }
    return true;
}

}
