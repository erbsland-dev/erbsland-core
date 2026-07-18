// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../mem/ByteBlock.hpp"
#include "../../../mem/ByteReader.hpp"
#include "../../../unit/ByteIndex.hpp"
#include "../../../unit/ByteLength.hpp"
#include "../../Char.hpp"
#include "../../EncodingErrorMode.hpp"
#include "../../impl/ThrowHelper.hpp"

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>

namespace erbsland::text::impl::utf8 {

/// Get the UTF-8 sequence length of this code-point.
/// Returns zero for invalid Unicode code-points.
/// @return The length of the UTF-8 sequence for this code-point.
[[nodiscard]] constexpr auto encodedLength(const Char character) noexcept -> unit::ByteLength {
    if (!character.isValidUnicode()) {
        return unit::ByteLength{};
    }
    if (character.toRawValue() <= 0x7FU) {
        return unit::ByteLength{1U};
    }
    if (character.toRawValue() <= 0x7FFU) {
        return unit::ByteLength{2U};
    }
    if (character.toRawValue() <= 0xFFFFU) {
        return unit::ByteLength{3U};
    }
    return unit::ByteLength{4U};
}

/// Decode the continuation byte of a multi-byte UTF-8 character.
/// @throws text::U8EncodingError On any UTF-8 encoding error.
/// @tested{U8EncodingTest}
inline void decodeCharOrThrow_decodeCont(
    const std::span<const char> buffer, const std::size_t index, char32_t &unicodeValue) {
    if (index >= buffer.size()) {
        text::impl::throwU8EncodingError("Unexpected end of the data", buffer.size());
    }
    const auto decodedByte = static_cast<std::uint8_t>(buffer[index]);
    if ((decodedByte & 0b11000000U) != 0b10000000U) {
        text::impl::throwU8EncodingError("Unexpected continuation byte", index);
    }
    unicodeValue <<= 6;
    unicodeValue |= static_cast<char32_t>(decodedByte & 0b00111111U);
}

/// Decode a single UTF-8 character in the buffer and advance the position.
/// On decoding error, throws an exception, position is unchanged.
/// @param buffer The byte buffer to decode.
/// @param position The read position that is advanced after a successful read.
/// @throws text::U8EncodingError On any UTF-8 encoding error.
/// @tested{U8EncodingTest}
inline auto decodeCharOrThrow(const std::span<const char> buffer, unit::ByteIndex &position) -> Char {
    auto index = position.toSizeT();
    if (index >= buffer.size()) {
        text::impl::throwOutOfRange("Read position out of range");
    }
    const auto decodedByte = static_cast<std::uint8_t>(buffer[index]);
    if (decodedByte < 0x80U) { // 7-bit ASCII?
        position.advance(unit::ByteLength{1U});
        return Char{static_cast<char32_t>(decodedByte)};
    }
    char32_t unicodeValue{};
    if ((decodedByte & 0b11100000U) == 0b11000000U && decodedByte >= 0b11000010U) { // 2-byte sequence
        unicodeValue = static_cast<char32_t>(decodedByte & 0b00011111U);
        decodeCharOrThrow_decodeCont(buffer, index + 1U, unicodeValue);
        index += 2U;
    } else if ((decodedByte & 0b11110000U) == 0b11100000U) { // 3-byte sequence
        unicodeValue = static_cast<char32_t>(decodedByte & 0b00001111U);
        decodeCharOrThrow_decodeCont(buffer, index + 1U, unicodeValue);
        decodeCharOrThrow_decodeCont(buffer, index + 2U, unicodeValue);
        if (unicodeValue < 0x800U) {
            text::impl::throwU8EncodingError("Overlong encoding", position.toSizeT());
        }
        index += 3U;
    } else if ((decodedByte & 0b11111000U) == 0b11110000U && decodedByte < 0b11110101U) { // 4-byte sequence
        unicodeValue = static_cast<char32_t>(decodedByte & 0b00000111U);
        decodeCharOrThrow_decodeCont(buffer, index + 1U, unicodeValue);
        decodeCharOrThrow_decodeCont(buffer, index + 2U, unicodeValue);
        decodeCharOrThrow_decodeCont(buffer, index + 3U, unicodeValue);
        if (unicodeValue < 0x10000U) { // 5+-byte sequence
            text::impl::throwU8EncodingError("Overlong encoding", position.toSizeT());
        }
        index += 4U;
    } else {
        text::impl::throwU8EncodingError("Invalid or out-of-range start byte sequence", position.toSizeT());
    }
    const auto result = Char{unicodeValue};
    if (!result.isValidUnicode()) {
        text::impl::throwU8EncodingError("Invalid Unicode character", position.toSizeT());
    }
    position = unit::ByteIndex{index};
    return result;
}

/// Decode a single UTF-8 character in the buffer and advance the position.
/// On encoding errors, returns the replacement character and moves the position by one byte.
/// For byte-structured sequences that encode invalid Unicode code-points, this method returns the replacement
/// character but moves the position by the encoded length.
/// @param buffer The byte buffer to decode.
/// @param position The read position that is advanced after a successful read.
/// @return The decoded character or replacement character.
[[nodiscard]] inline auto decodeCharOrReplace(const std::span<const char> buffer, unit::ByteIndex &position) noexcept
    -> Char {
    auto index = position.toSizeT();
    if (index >= buffer.size()) {
        return Char{0U};
    }
    auto decodedByte = static_cast<std::uint8_t>(buffer[index]);
    index += 1;
    if (decodedByte < 0x80U) { // 7-bit ASCII?
        position = unit::ByteIndex::fromSizeT(index);
        return Char{static_cast<char32_t>(decodedByte)};
    }
    uint8_t sequenceSize = 0;
    char32_t unicodeValue{};
    if ((decodedByte & 0b11100000U) == 0b11000000U && decodedByte >= 0b11000010U) {
        sequenceSize = 2; // 2-byte sequence
        unicodeValue = static_cast<char32_t>(decodedByte & 0b00011111U);
    } else if ((decodedByte & 0b11110000U) == 0b11100000U) {
        sequenceSize = 3; // 3-byte sequence
        unicodeValue = static_cast<char32_t>(decodedByte & 0b00001111U);
    } else if ((decodedByte & 0b11111000U) == 0b11110000U && decodedByte < 0b11110101U) {
        sequenceSize = 4; // 4-byte sequence
        unicodeValue = static_cast<char32_t>(decodedByte & 0b00000111U);
    } else {
        position.increment();
        return Char::replacement();
    }
    for (uint8_t i = 1; i < sequenceSize; ++i) {
        if (index >= buffer.size()) {
            position.increment();
            return Char::replacement();
        }
        decodedByte = static_cast<std::uint8_t>(buffer[index]);
        if ((decodedByte & 0b11000000U) != 0b10000000U) {
            position.increment();
            return Char::replacement();
        }
        index += 1;
        unicodeValue <<= 6;
        unicodeValue |= static_cast<char32_t>(decodedByte & 0b00111111U);
    }
    if ((sequenceSize == 3 && unicodeValue < 0x800) || (sequenceSize == 4 && unicodeValue < 0x10000)) {
        position.increment();
        return Char::replacement();
    }
    position = unit::ByteIndex::fromSizeT(index);
    const auto result = Char{unicodeValue};
    if (!result.isValidUnicode()) {
        return Char::replacement();
    }
    return result;
}

/// Decode a single UTF-8 character in the buffer and advance the position.
/// Works like `decodeCharOrReplace` but returns no character in error cases.
/// @param buffer The byte buffer to decode.
/// @param position The read position that is advanced after a successful read.
/// @return A valid decoded character or no character in case of an error.
[[nodiscard]] inline auto decodeCharOrIgnore(const std::span<const char> buffer, unit::ByteIndex &position) noexcept
    -> std::optional<Char> {
    auto index = position.toSizeT();
    if (index >= buffer.size()) {
        return std::nullopt;
    }
    auto decodedByte = static_cast<std::uint8_t>(buffer[index]);
    index += 1;
    if (decodedByte < 0x80U) { // 7-bit ASCII?
        position = unit::ByteIndex::fromSizeT(index);
        return Char{static_cast<char32_t>(decodedByte)};
    }
    uint8_t sequenceSize = 0;
    char32_t unicodeValue{};
    if ((decodedByte & 0b11100000U) == 0b11000000U && decodedByte >= 0b11000010U) {
        sequenceSize = 2; // 2-byte sequence
        unicodeValue = static_cast<char32_t>(decodedByte & 0b00011111U);
    } else if ((decodedByte & 0b11110000U) == 0b11100000U) {
        sequenceSize = 3; // 3-byte sequence
        unicodeValue = static_cast<char32_t>(decodedByte & 0b00001111U);
    } else if ((decodedByte & 0b11111000U) == 0b11110000U && decodedByte < 0b11110101U) {
        sequenceSize = 4; // 4-byte sequence
        unicodeValue = static_cast<char32_t>(decodedByte & 0b00000111U);
    } else {
        position.increment();
        return std::nullopt;
    }
    for (uint8_t i = 1; i < sequenceSize; ++i) {
        if (index >= buffer.size()) {
            position.increment();
            return std::nullopt;
        }
        decodedByte = static_cast<std::uint8_t>(buffer[index]);
        if ((decodedByte & 0b11000000U) != 0b10000000U) {
            position.increment();
            return std::nullopt;
        }
        index += 1;
        unicodeValue <<= 6;
        unicodeValue |= static_cast<char32_t>(decodedByte & 0b00111111U);
    }
    if ((sequenceSize == 3 && unicodeValue < 0x800) || (sequenceSize == 4 && unicodeValue < 0x10000)) {
        position.increment();
        return std::nullopt;
    }
    position = unit::ByteIndex::fromSizeT(index);
    const auto result = Char{unicodeValue};
    if (!result.isValidUnicode()) {
        return std::nullopt;
    }
    return result;
}

/// Fast advancing the position index in the given buffer.
/// If the index is at the end or out of bounds, this function does nothing.
/// This method follows the same movement pattern as the tolerant decoder:
/// - ASCII and accepted byte-structured UTF-8 sequences advance by their encoded length.
/// - Encoding errors advance by one byte.
/// - Byte-structured sequences for invalid Unicode code-points are skipped as one sequence.
/// @param buffer The buffer.
/// @param position The position index to advance.
inline void fastAdvanceChar(const std::span<const char> buffer, unit::ByteIndex &position) {
    auto index = position.toSizeT();
    if (index >= buffer.size() || buffer.empty()) {
        return;
    }
    const auto isContinuationByte = [&](const std::size_t delta) noexcept -> bool {
        return index + delta < buffer.size() &&
            (static_cast<std::uint8_t>(buffer[index + delta]) & 0b11000000U) == 0b10000000U;
    };
    const auto byteAt = [&](const std::size_t delta) noexcept -> std::uint8_t {
        return static_cast<std::uint8_t>(buffer[index + delta]);
    };
    auto decodedByte = static_cast<std::uint8_t>(buffer[index]);
    unit::ByteLength advanceLength{1};
    if (decodedByte < 0x80U || (decodedByte & 0b11111000U) == 0b11111000U) {
        // advanced 1-byte
    } else if ((decodedByte & 0b11100000U) == 0b11000000U) {
        if (decodedByte >= 0b11000010U && isContinuationByte(1U)) {
            advanceLength = unit::ByteLength{2};
        }
    } else if ((decodedByte & 0b11110000U) == 0b11100000U) {
        if (isContinuationByte(1U) && isContinuationByte(2U) && (decodedByte != 0xE0U || byteAt(1U) >= 0xA0U)) {
            advanceLength = unit::ByteLength{3};
        }
    } else if ((decodedByte & 0b11111000U) == 0b11110000U) {
        if (decodedByte < 0b11110101U && isContinuationByte(1U) && isContinuationByte(2U) && isContinuationByte(3U) &&
            (decodedByte != 0xF0U || byteAt(1U) >= 0x90U)) {
            advanceLength = unit::ByteLength{4};
        }
    }
    position.advance(advanceLength);
}

/// Fast retreating the position index in the given buffer.
/// If the index is at zero, this function does nothing.
/// If the index is out-of-bounds, it is assumed to be at the end.
/// This function only looks at up to four bytes **in front** of the current position.
/// The bytes must build an accepted byte-structured UTF-8 sequence that ends with the current byte.
/// Encoding errors retreat by one byte. Byte-structured sequences for invalid Unicode code-points are skipped as
/// one sequence.
inline void fastRetreatChar(const std::span<const char> buffer, unit::ByteIndex &position) {
    auto index = std::min(position.toSizeT(), buffer.size());
    if (index == 0 || buffer.empty()) {
        return;
    }
    // This is not the fastest possible implementation, but I hope a compiler can optimize it well.
    // I'm scanning up to 4 bytes backwards and test for each if it is a continuation byte.
    // - When encountering a continuation byte, I need to test more bytes.
    // - When I found no non-continuation byte after 4 bytes, it's an encoding error, so I step back one.
    // - Do I find a non-continuation byte, I test if it is a valid start byte.
    // - Also, if it marks a byte sequence, the length must match the distance to the current position.
    // - Only if start sequence and distance match, and there is a high chance that we got a valid,
    //   UTF-8 sequence, another length than one is returned.
    const auto isContinuationByte = [&](const std::size_t delta) noexcept -> bool {
        assert(index >= delta);
        return (static_cast<std::uint8_t>(buffer[index - delta]) & 0b11000000U) == 0b10000000U;
    };
    const auto lengthByte = [&](const std::size_t delta) -> std::size_t {
        if (index < delta) {
            return 1U; // error
        }
        const auto startIndex = index - delta;
        auto decodedByte = static_cast<std::uint8_t>(buffer[startIndex]);
        if (isContinuationByte(delta)) {
            return 0U; // need to test more.
        }
        if (decodedByte < 0x80U || (decodedByte & 0b11111000U) == 0b11111000U) {
            return 1U; // ascii or error 5-byte sequence.
        }
        if ((decodedByte & 0b11100000U) == 0b11000000U && delta == 2 && decodedByte >= 0b11000010U) {
            return 2U;
        }
        if ((decodedByte & 0b11110000U) == 0b11100000U && delta == 3 &&
            (decodedByte != 0xE0U || static_cast<std::uint8_t>(buffer[startIndex + 1U]) >= 0xA0U)) {
            return 3U;
        }
        if ((decodedByte & 0b11111000U) == 0b11110000U && delta == 4 && decodedByte < 0b11110101U &&
            (decodedByte != 0xF0U || static_cast<std::uint8_t>(buffer[startIndex + 1U]) >= 0x90U)) {
            return 4U;
        }
        return 1U; // error
    };
    for (std::size_t delta = 1U; delta <= 4U; ++delta) {
        if (const auto result = lengthByte(delta); result > 0) {
            position.retreat(unit::ByteLength::fromSizeT(result));
            return;
        }
    }
    position.retreat(unit::ByteLength{1U});
}

/// Decode all UTF-8 characters with the selected error handling mode and call the function for each result.
/// Stops when the callback returns `false`.
/// @tested{U8EncodingTest}
template <EncodingErrorMode errorMode, typename Function>
auto forEachDecodedCharacter(const std::span<const char> data, Function function) -> bool {
    auto position = unit::ByteIndex::zero();
    while (position.toSizeT() < data.size()) {
        auto character = Char{};
        if constexpr (errorMode == EncodingErrorMode::Throw) {
            character = decodeCharOrThrow(data, position);
        } else if constexpr (errorMode == EncodingErrorMode::Ignore) {
            if (const auto optChar = decodeCharOrIgnore(data, position); optChar.has_value()) {
                character = *optChar;
            } else {
                continue;
            }
        } else {
            character = decodeCharOrReplace(data, position);
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

/// Decode all UTF-8 characters with a runtime selected error handling mode.
/// @tested{U8EncodingTest}
template <typename Function>
auto forEachDecodedCharacter(const std::span<const char> data, const EncodingErrorMode errorMode, Function function)
    -> bool {
    switch (errorMode) {
    case EncodingErrorMode::Throw:
        return forEachDecodedCharacter<EncodingErrorMode::Throw>(data, function);
    case EncodingErrorMode::Ignore:
        return forEachDecodedCharacter<EncodingErrorMode::Ignore>(data, function);
    case EncodingErrorMode::Replace:
        return forEachDecodedCharacter<EncodingErrorMode::Replace>(data, function);
    }
    return false;
}

/// Decode the continuation byte of a multi-byte UTF-8 character from a byte reader.
/// @throws text::U8EncodingError On any UTF-8 encoding error.
/// @tested{U8StringEncodingTest}
inline void decodeCharOrThrow_decodeCont(
    const mem::ByteReader &reader, const std::size_t offset, char32_t &unicodeValue) {
    if (!reader.canRead(offset + 1U)) {
        text::impl::throwU8EncodingError("Unexpected end of the data", reader.length().toSizeT());
    }
    const auto decodedByte = reader.peekByte(offset);
    if (!decodedByte.matches(0b11000000U, 0b10000000U)) {
        text::impl::throwU8EncodingError("Unexpected continuation byte", reader.position().toSizeT() + offset);
    }
    unicodeValue <<= 6;
    unicodeValue |= static_cast<char32_t>(decodedByte.masked(0b00111111U));
}

/// Decode a single UTF-8 character from a byte reader and advance the position.
/// @throws text::U8EncodingError On any UTF-8 encoding error.
/// @tested{U8StringEncodingTest}
inline auto decodeCharOrThrow(mem::ByteReader &reader) -> Char {
    const auto startPosition = reader.position();
    if (!reader.canRead(1U)) {
        text::impl::throwOutOfRange("Read position out of range");
    }
    const auto decodedByte = reader.peekByte();
    if (decodedByte.toUInt8() < 0x80U) {
        reader.advance(1U);
        return Char{static_cast<char32_t>(decodedByte.toUInt8())};
    }
    char32_t unicodeValue{};
    auto sequenceSize = std::size_t{0};
    if (decodedByte.matches(0b11100000U, 0b11000000U) && decodedByte.toUInt8() >= 0b11000010U) {
        unicodeValue = static_cast<char32_t>(decodedByte.masked(0b00011111U));
        decodeCharOrThrow_decodeCont(reader, 1U, unicodeValue);
        sequenceSize = 2U;
    } else if (decodedByte.matches(0b11110000U, 0b11100000U)) {
        unicodeValue = static_cast<char32_t>(decodedByte.masked(0b00001111U));
        decodeCharOrThrow_decodeCont(reader, 1U, unicodeValue);
        decodeCharOrThrow_decodeCont(reader, 2U, unicodeValue);
        if (unicodeValue < 0x800U) {
            text::impl::throwU8EncodingError("Overlong encoding", startPosition.toSizeT());
        }
        sequenceSize = 3U;
    } else if (decodedByte.matches(0b11111000U, 0b11110000U) && decodedByte.toUInt8() < 0b11110101U) {
        unicodeValue = static_cast<char32_t>(decodedByte.masked(0b00000111U));
        decodeCharOrThrow_decodeCont(reader, 1U, unicodeValue);
        decodeCharOrThrow_decodeCont(reader, 2U, unicodeValue);
        decodeCharOrThrow_decodeCont(reader, 3U, unicodeValue);
        if (unicodeValue < 0x10000U) {
            text::impl::throwU8EncodingError("Overlong encoding", startPosition.toSizeT());
        }
        sequenceSize = 4U;
    } else {
        text::impl::throwU8EncodingError("Invalid or out-of-range start byte sequence", startPosition.toSizeT());
    }
    const auto result = Char{unicodeValue};
    if (!result.isValidUnicode()) {
        text::impl::throwU8EncodingError("Invalid Unicode character", startPosition.toSizeT());
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
        return Char{static_cast<char32_t>(decodedByte.toUInt8())};
    }
    auto sequenceSize = std::size_t{0};
    char32_t unicodeValue{};
    if (decodedByte.matches(0b11100000U, 0b11000000U) && decodedByte.toUInt8() >= 0b11000010U) {
        sequenceSize = 2U;
        unicodeValue = static_cast<char32_t>(decodedByte.masked(0b00011111U));
    } else if (decodedByte.matches(0b11110000U, 0b11100000U)) {
        sequenceSize = 3U;
        unicodeValue = static_cast<char32_t>(decodedByte.masked(0b00001111U));
    } else if (decodedByte.matches(0b11111000U, 0b11110000U) && decodedByte.toUInt8() < 0b11110101U) {
        sequenceSize = 4U;
        unicodeValue = static_cast<char32_t>(decodedByte.masked(0b00000111U));
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
        unicodeValue |= static_cast<char32_t>(continuationByte.masked(0b00111111U));
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
[[nodiscard]] inline auto decodeCharOrIgnore(mem::ByteReader &reader) noexcept -> std::optional<Char> {
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
        unicodeValue = static_cast<char32_t>(decodedByte.masked(0b00011111U));
    } else if (decodedByte.matches(0b11110000U, 0b11100000U)) {
        sequenceSize = 3U;
        unicodeValue = static_cast<char32_t>(decodedByte.masked(0b00001111U));
    } else if (decodedByte.matches(0b11111000U, 0b11110000U) && decodedByte.toUInt8() < 0b11110101U) {
        sequenceSize = 4U;
        unicodeValue = static_cast<char32_t>(decodedByte.masked(0b00000111U));
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
        unicodeValue |= static_cast<char32_t>(continuationByte.masked(0b00111111U));
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
template <EncodingErrorMode errorMode, typename Function>
auto forEachDecodedCharacter(mem::ByteReader &reader, Function function) -> bool {
    while (!reader.isAtEnd()) {
        auto character = Char{};
        if constexpr (errorMode == EncodingErrorMode::Throw) {
            character = decodeCharOrThrow(reader);
        } else if constexpr (errorMode == EncodingErrorMode::Ignore) {
            if (const auto optChar = decodeCharOrIgnore(reader); optChar.has_value()) {
                character = *optChar;
            } else {
                continue;
            }
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
auto forEachDecodedCharacter(mem::ByteReader &reader, const EncodingErrorMode errorMode, Function function) -> bool {
    switch (errorMode) {
    case EncodingErrorMode::Throw:
        return forEachDecodedCharacter<EncodingErrorMode::Throw>(reader, function);
    case EncodingErrorMode::Ignore:
        return forEachDecodedCharacter<EncodingErrorMode::Ignore>(reader, function);
    case EncodingErrorMode::Replace:
        return forEachDecodedCharacter<EncodingErrorMode::Replace>(reader, function);
    }
    return false;
}

/// Test if the UTF-8 byte sequence is fully valid.
[[nodiscard]] inline auto isValid(const std::span<const char> data) noexcept -> bool {
    auto position = unit::ByteIndex::zero();
    while (position.toSizeT() < data.size()) {
        if (!decodeCharOrIgnore(data, position).has_value()) {
            return false;
        }
    }
    return true;
}

}
