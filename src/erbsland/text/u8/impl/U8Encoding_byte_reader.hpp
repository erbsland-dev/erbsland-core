// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8Encoding.hpp"

namespace erbsland::text::impl::utf8 {
/// Decode the continuation byte of a multi-byte UTF-8 character from a byte reader.
/// @throws text::U8EncodingError On any UTF-8 encoding error.
/// @tested{U8StringEncodingTest}
inline void decodeCharOrThrow_decodeCont(
    const mem::ByteReader &reader, const std::size_t offset, char32_t &unicodeValue) {
    if (!reader.canRead(offset + 1U)) {
        throwU8EncodingError("Unexpected end of the data", reader.length().toSizeT());
    }
    const auto decodedByte = reader.peekByte(offset);
    if (!decodedByte.matches(0b11000000U, 0b10000000U)) {
        throwU8EncodingError("Unexpected continuation byte", reader.position().toSizeT() + offset);
    }
    unicodeValue <<= 6;
    unicodeValue |= decodedByte.masked(0b00111111U).toUInt32();
}

/// Decode a single UTF-8 character from a byte reader and advance the position.
/// @throws text::U8EncodingError On any UTF-8 encoding error.
/// @tested{U8StringEncodingTest}
inline auto decodeCharOrThrow(mem::ByteReader &reader) -> Char {
    const auto startPosition = reader.position();
    if (!reader.canRead(1U)) {
        throwOutOfRange("Read position out of range");
    }
    const auto decodedByte = reader.peekByte();
    if (decodedByte.toUInt8() < 0x80U) {
        reader.advance(1U);
        return Char{static_cast<char32_t>(decodedByte.toUInt32())};
    }
    char32_t unicodeValue{};
    auto sequenceSize = std::size_t{0};
    if (decodedByte.matches(0b11100000U, 0b11000000U) && decodedByte.toUInt8() >= 0b11000010U) {
        unicodeValue = static_cast<char32_t>(decodedByte.masked(0b00011111U).toUInt32());
        decodeCharOrThrow_decodeCont(reader, 1U, unicodeValue);
        sequenceSize = 2U;
    } else if (decodedByte.matches(0b11110000U, 0b11100000U)) {
        unicodeValue = static_cast<char32_t>(decodedByte.masked(0b00001111U).toUInt32());
        decodeCharOrThrow_decodeCont(reader, 1U, unicodeValue);
        decodeCharOrThrow_decodeCont(reader, 2U, unicodeValue);
        if (unicodeValue < 0x800U) {
            throwU8EncodingError("Overlong encoding", startPosition.toSizeT());
        }
        sequenceSize = 3U;
    } else if (decodedByte.matches(0b11111000U, 0b11110000U) && decodedByte.toUInt8() < 0b11110101U) {
        unicodeValue = static_cast<char32_t>(decodedByte.masked(0b00000111U).toUInt32());
        decodeCharOrThrow_decodeCont(reader, 1U, unicodeValue);
        decodeCharOrThrow_decodeCont(reader, 2U, unicodeValue);
        decodeCharOrThrow_decodeCont(reader, 3U, unicodeValue);
        if (unicodeValue < 0x10000U) {
            throwU8EncodingError("Overlong encoding", startPosition.toSizeT());
        }
        sequenceSize = 4U;
    } else {
        throwU8EncodingError("Invalid or out-of-range start byte sequence", startPosition.toSizeT());
    }
    const auto result = Char{unicodeValue};
    if (!result.isValidUnicode()) {
        throwU8EncodingError("Invalid Unicode character", startPosition.toSizeT());
    }
    reader.setPosition(startPosition + unit::ByteLength::fromSizeT(sequenceSize));
    return result;
}

/// Decode a single UTF-8 character from a byte reader and advance the position.
[[nodiscard]] inline auto decodeCharOrReplace(mem::ByteReader &reader) noexcept -> Char {
    if (!reader.canRead(1U)) {
        return Char{0U};
    }
    const auto decodedByte = reader.peekByte();
    if (decodedByte.toUInt8() < 0x80U) {
        reader.advance(1U);
        return Char{static_cast<char32_t>(decodedByte.toUInt32())};
    }
    auto sequenceSize = std::size_t{0};
    char32_t unicodeValue{};
    if (decodedByte.matches(0b11100000U, 0b11000000U) && decodedByte.toUInt8() >= 0b11000010U) {
        sequenceSize = 2U;
        unicodeValue = static_cast<char32_t>(decodedByte.masked(0b00011111U).toUInt32());
    } else if (decodedByte.matches(0b11110000U, 0b11100000U)) {
        sequenceSize = 3U;
        unicodeValue = static_cast<char32_t>(decodedByte.masked(0b00001111U).toUInt32());
    } else if (decodedByte.matches(0b11111000U, 0b11110000U) && decodedByte.toUInt8() < 0b11110101U) {
        sequenceSize = 4U;
        unicodeValue = static_cast<char32_t>(decodedByte.masked(0b00000111U).toUInt32());
    } else {
        reader.advance(1U);
        return Char::replacement();
    }
    for (auto i = std::size_t{1U}; i < sequenceSize; ++i) {
        if (!reader.canRead(i + 1U)) {
            reader.advance(1U);
            return Char::replacement();
        }
        const auto continuationByte = reader.peekByte(i);
        if (!continuationByte.matches(0b11000000U, 0b10000000U)) {
            reader.advance(1U);
            return Char::replacement();
        }
        unicodeValue <<= 6U;
        unicodeValue |= static_cast<char32_t>(continuationByte.masked(0b00111111U).toUInt32());
    }
    if ((sequenceSize == 3U && unicodeValue < 0x800U) || (sequenceSize == 4U && unicodeValue < 0x10000U)) {
        reader.advance(1U);
        return Char::replacement();
    }
    const auto result = Char{unicodeValue};
    if (!result.isValidUnicode()) {
        reader.advance(sequenceSize);
        return Char::replacement();
    }
    reader.advance(sequenceSize);
    return result;
}

/// Decode a single UTF-8 character from a byte reader and advance the position.
[[nodiscard]] inline auto tryDecodeChar(mem::ByteReader &reader) noexcept -> std::optional<Char> {
    if (!reader.canRead(1U)) {
        return std::nullopt;
    }
    const auto decodedByte = reader.peekByte();
    if (decodedByte.toUInt8() < 0x80U) {
        reader.advance(1U);
        return Char{static_cast<char32_t>(decodedByte.toUInt8())};
    }
    auto sequenceSize = std::size_t{0};
    char32_t unicodeValue{};
    if (decodedByte.matches(0b11100000U, 0b11000000U) && decodedByte.toUInt8() >= 0b11000010U) {
        sequenceSize = 2U;
        unicodeValue = static_cast<char32_t>(decodedByte.masked(0b00011111U).toUInt32());
    } else if (decodedByte.matches(0b11110000U, 0b11100000U)) {
        sequenceSize = 3U;
        unicodeValue = static_cast<char32_t>(decodedByte.masked(0b00001111U).toUInt32());
    } else if (decodedByte.matches(0b11111000U, 0b11110000U) && decodedByte.toUInt8() < 0b11110101U) {
        sequenceSize = 4U;
        unicodeValue = static_cast<char32_t>(decodedByte.masked(0b00000111U).toUInt32());
    } else {
        reader.advance(1U);
        return std::nullopt;
    }
    for (auto i = std::size_t{1U}; i < sequenceSize; ++i) {
        if (!reader.canRead(i + 1U)) {
            reader.advance(1U);
            return std::nullopt;
        }
        const auto continuationByte = reader.peekByte(i);
        if (!continuationByte.matches(0b11000000U, 0b10000000U)) {
            reader.advance(1U);
            return std::nullopt;
        }
        unicodeValue <<= 6U;
        unicodeValue |= static_cast<char32_t>(continuationByte.masked(0b00111111U).toUInt32());
    }
    if ((sequenceSize == 3U && unicodeValue < 0x800U) || (sequenceSize == 4U && unicodeValue < 0x10000U)) {
        reader.advance(1U);
        return std::nullopt;
    }
    const auto result = Char{unicodeValue};
    if (!result.isValidUnicode()) {
        reader.advance(sequenceSize);
        return std::nullopt;
    }
    reader.advance(sequenceSize);
    return result;
}

/// Decode all UTF-8 characters from a byte reader with the selected error handling mode.
/// @tested{U8StringEncodingTest}
template <EncodingMode mode, typename Function>
auto forEachDecodedCharacter(mem::ByteReader &reader, Function function) -> bool {
    while (!reader.isAtEnd()) {
        auto character = Char{};
        if constexpr (mode == EncodingMode::Strict) {
            character = decodeCharOrThrow(reader);
        } else {
            character = decodeCharOrReplace(reader);
        }
        if constexpr (std::same_as<std::invoke_result_t<Function, Char>, bool>) {
            if (!function(character)) {
                return false;
            }
        } else {
            function(character);
        }
    }
    return true;
}

/// Decode all UTF-8 characters from a byte reader with a runtime selected error handling mode.
/// @tested{U8StringEncodingTest}
template <typename Function>
auto forEachDecodedCharacter(mem::ByteReader &reader, const EncodingMode mode, Function function) -> bool {
    switch (mode) {
    case EncodingMode::Strict:
        return forEachDecodedCharacter<EncodingMode::Strict>(reader, function);
    case EncodingMode::Tolerant:
        return forEachDecodedCharacter<EncodingMode::Tolerant>(reader, function);
    }
    return false;
}

/// Test if the UTF-8 byte sequence is fully valid.
[[nodiscard]] inline auto isValid(const std::span<const char> data) noexcept -> bool {
    auto position = unit::ByteIndex::zero();
    while (position.toSizeT() < data.size()) {
        if (!tryDecodeChar(data, position).has_value()) {
            return false;
        }
    }
    return true;
}

}
