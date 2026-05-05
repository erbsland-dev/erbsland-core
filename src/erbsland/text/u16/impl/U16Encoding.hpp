// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../err/ThrowHelper.hpp"
#include "../../../mem/ByteBlockView.hpp"
#include "../../../mem/ByteReader.hpp"
#include "../../../unit/U16DataIndex.hpp"
#include "../../../unit/U16DataLength.hpp"
#include "../../StringEncoding.hpp"
#include "../../u8/impl/U8Encoding.hpp"

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>

namespace erbsland::text::impl::utf16 {

/// Test if the string encoding is a UTF-16 encoding.
/// @tested{U8StringEncodingTest}
[[nodiscard]] constexpr auto isEncoding(const StringEncoding encoding) noexcept -> bool {
    return encoding == StringEncoding::Utf16 || encoding == StringEncoding::Utf16LittleEndian ||
        encoding == StringEncoding::Utf16BigEndian;
}

/// Get the UTF-16 byte order mark length.
/// @tested{U8StringEncodingTest}
[[nodiscard]] constexpr auto bomLength() noexcept -> std::size_t {
    return 2U;
}

/// Test if the byte data starts with a UTF-16 little-endian byte order mark.
/// @tested{U8StringEncodingTest}
[[nodiscard]] inline auto hasLittleEndianBom(const mem::ByteBlockView &data) noexcept -> bool {
    return data.startsWith({0xFFU, 0xFEU});
}

/// Test if the byte data starts with a UTF-16 big-endian byte order mark.
/// @tested{U8StringEncodingTest}
[[nodiscard]] inline auto hasBigEndianBom(const mem::ByteBlockView &data) noexcept -> bool {
    return data.startsWith({0xFEU, 0xFFU});
}

/// Read one UTF-16 code unit from a byte reader.
/// @tested{U8StringEncodingTest}
[[nodiscard]] inline auto readCodeUnit(mem::ByteReader &reader) noexcept -> char16_t {
    return static_cast<char16_t>(reader.readUInt16());
}

/// Get the UTF-16 encoded length
/// @tested{U16EncodingTest}
inline auto encodedLength(const Char character) noexcept -> unit::U16DataLength {
    if (!character.isValidUnicode()) {
        return unit::U16DataLength::one();
    }
    const auto codePoint = character.toRawValue();
    if (codePoint <= 0xFFFFU) {
        return unit::U16DataLength::one();
    }
    return unit::U16DataLength{2U};
}

/// Decode a single UTF-16 character in the buffer and advance the position.
/// On decoding error, throws an exception, position is unchanged.
/// @tested{U16EncodingTest}
inline auto decodeCharOrThrow(const std::span<const char16_t> buffer, unit::U16DataIndex &position) -> Char {
    const auto index = position.toSizeT();
    if (index >= buffer.size()) {
        err::throwOutOfRange("Read position out of range");
    }
    const auto unit = buffer[index];
    if (Char::isHighSurrogate(unit)) {
        if (index + 1U >= buffer.size() || !Char::isLowSurrogate(buffer[index + 1U])) {
            err::throwU16EncodingError("Invalid UTF-16 high surrogate", index);
        }
        const char32_t codePoint = char32_t{0x10000U} + (static_cast<char32_t>(unit - 0xD800U) << 10U) +
            static_cast<char32_t>(buffer[index + 1U] - 0xDC00U);
        position.advance(unit::U16DataLength{2U});
        return Char{codePoint};
    }
    if (Char::isLowSurrogate(unit)) {
        err::throwU16EncodingError("Invalid UTF-16 low surrogate", index);
    }
    position.advance(unit::U16DataLength::one());
    return Char{static_cast<char32_t>(unit)};
}

/// Decode a single UTF-16 character in the buffer and advance the position.
/// Malformed surrogate units are replaced and advance by one code unit.
/// @tested{U16EncodingTest}
[[nodiscard]] inline auto decodeCharOrReplace(
    const std::span<const char16_t> buffer, unit::U16DataIndex &position) noexcept -> Char {
    const auto index = position.toSizeT();
    if (index >= buffer.size()) {
        return Char{0U};
    }
    const auto unit = buffer[index];
    if (Char::isHighSurrogate(unit)) {
        if (index + 1U < buffer.size() && Char::isLowSurrogate(buffer[index + 1U])) {
            const char32_t codePoint = char32_t{0x10000U} + (static_cast<char32_t>(unit - 0xD800U) << 10U) +
                static_cast<char32_t>(buffer[index + 1U] - 0xDC00U);
            position.advance(unit::U16DataLength{2U});
            return Char{codePoint};
        }
        position.increment();
        return Char::replacement();
    }
    if (Char::isLowSurrogate(unit)) {
        position.increment();
        return Char::replacement();
    }
    position.increment();
    return Char{static_cast<char32_t>(unit)};
}

/// Decode a single UTF-16 character in the buffer and advance the position.
/// Malformed surrogate units return no character and advance by one code unit.
/// @tested{U16EncodingTest}
[[nodiscard]] inline auto decodeCharOrIgnore(
    const std::span<const char16_t> buffer, unit::U16DataIndex &position) noexcept -> std::optional<Char> {
    const auto index = position.toSizeT();
    if (index >= buffer.size()) {
        return std::nullopt;
    }
    const auto unit = buffer[index];
    if (Char::isHighSurrogate(unit)) {
        if (index + 1U < buffer.size() && Char::isLowSurrogate(buffer[index + 1U])) {
            const char32_t codePoint = char32_t{0x10000U} + (static_cast<char32_t>(unit - 0xD800U) << 10U) +
                static_cast<char32_t>(buffer[index + 1U] - 0xDC00U);
            position.advance(unit::U16DataLength{2U});
            return Char{codePoint};
        }
        position.increment();
        return std::nullopt;
    }
    if (Char::isLowSurrogate(unit)) {
        position.increment();
        return std::nullopt;
    }
    position.increment();
    return Char{static_cast<char32_t>(unit)};
}

/// Fast advancing the position index in the given UTF-16 buffer.
/// Malformed surrogate units advance by one code unit.
inline void fastAdvanceChar(const std::span<const char16_t> buffer, unit::U16DataIndex &position) noexcept {
    const auto index = position.toSizeT();
    if (index >= buffer.size() || buffer.empty()) {
        return;
    }
    if (Char::isHighSurrogate(buffer[index]) && index + 1U < buffer.size() &&
        Char::isLowSurrogate(buffer[index + 1U])) {
        position.advance(unit::U16DataLength{2U});
        return;
    }
    position.increment();
}

/// Fast retreating the position index in the given UTF-16 buffer.
/// Malformed surrogate units retreat by one code unit.
inline void fastRetreatChar(const std::span<const char16_t> buffer, unit::U16DataIndex &position) noexcept {
    auto index = std::min(position.toSizeT(), buffer.size());
    if (index == 0U || buffer.empty()) {
        return;
    }
    if (index >= 2U && Char::isLowSurrogate(buffer[index - 1U]) && Char::isHighSurrogate(buffer[index - 2U])) {
        position = unit::U16DataIndex::fromSizeT(index - 2U);
        return;
    }
    position = unit::U16DataIndex::fromSizeT(index - 1U);
}

/// Iterate over a UTF-16 encoded string, decoding each character and handling errors according to the specified mode.
/// @tested{U16EncodingTest}
template <typename tFunction>
void forEachDecodedCharacter(const std::u16string_view text, const EncodingErrorMode errorMode, tFunction function) {
    for (auto index = std::size_t{0}; index < text.size(); ++index) {
        const auto unit = text[index];
        if (Char::isHighSurrogate(unit)) {
            if (index + 1U < text.size() && Char::isLowSurrogate(text[index + 1U])) {
                const char32_t codePoint = 0x10000U + (static_cast<char32_t>(unit - 0xD800U) << 10U) +
                    static_cast<char32_t>(text[index + 1U] - 0xDC00U);
                function(Char{codePoint});
                ++index;
                continue;
            }
            switch (errorMode) {
            case EncodingErrorMode::Throw:
                err::throwU16EncodingError("Invalid UTF-16 high surrogate", index);
            case EncodingErrorMode::Ignore:
                break;
            case EncodingErrorMode::Replace:
                function(Char::replacement());
                break;
            }
            continue;
        }
        if (Char::isLowSurrogate(unit)) {
            switch (errorMode) {
            case EncodingErrorMode::Throw:
                err::throwU16EncodingError("Invalid UTF-16 low surrogate", index);
            case EncodingErrorMode::Ignore:
                break;
            case EncodingErrorMode::Replace:
                function(Char::replacement());
                break;
            }
            continue;
        }
        function(Char{static_cast<char32_t>(unit)});
    }
}

/// Iterate over a UTF-16 encoded string, decoding each character and handling errors according to the specified mode.
/// @tested{U16EncodingTest}
template <typename tFunction>
void forEachDecodedCharacter(const std::u16string &text, const EncodingErrorMode errorMode, tFunction function) {
    forEachDecodedCharacter(std::u16string_view{text}, errorMode, function);
}

/// Decode all UTF-16 characters with the selected error handling mode and call the function for each result.
/// Stops when the callback returns `false`.
/// @tested{U16EncodingTest}
template <EncodingErrorMode errorMode, typename Function>
auto forEachDecodedCharacter(const std::span<const char16_t> data, Function function) -> bool {
    auto position = unit::U16DataIndex::zero();
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

/// Decode all UTF-16 characters with a runtime selected error handling mode.
/// @tested{U16EncodingTest}
template <typename Function>
auto forEachDecodedCharacter(const std::span<const char16_t> data, const EncodingErrorMode errorMode, Function function)
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

/// Test if the UTF-16 code-unit sequence is fully valid.
/// @tested{U16EncodingTest}
[[nodiscard]] inline auto isValid(const std::span<const char16_t> data) noexcept -> bool {
    auto position = unit::U16DataIndex::zero();
    while (position.toSizeT() < data.size()) {
        if (!decodeCharOrIgnore(data, position).has_value()) {
            return false;
        }
    }
    return true;
}

/// Iterate over UTF-16 encoded byte data from a byte reader, decoding each character.
/// @tested{U8StringEncodingTest}
template <EncodingErrorMode errorMode, typename Function>
auto forEachDecodedCharacter(mem::ByteReader &reader, Function function) -> bool {
    auto unitIndex = std::size_t{0};
    while (!reader.isAtEnd()) {
        if (!reader.canRead(2U)) {
            if constexpr (errorMode == EncodingErrorMode::Throw) {
                err::throwEncodingError("Truncated UTF-16 data");
            } else {
                if constexpr (errorMode == EncodingErrorMode::Replace) {
                    if constexpr (std::same_as<std::invoke_result_t<Function, Char>, bool>) {
                        if (!function(Char::replacement())) {
                            return false;
                        }
                    } else {
                        function(Char::replacement());
                    }
                }
                reader.advance(2U);
                continue;
            }
        }

        const auto unitPosition = reader.position();
        const auto unit = readCodeUnit(reader);
        if (Char::isHighSurrogate(unit)) {
            if (reader.canRead(2U)) {
                auto lowReader = reader;
                const auto low = readCodeUnit(lowReader);
                if (Char::isLowSurrogate(low)) {
                    const char32_t codePoint = 0x10000U + (static_cast<char32_t>(unit - 0xD800U) << 10U) +
                        static_cast<char32_t>(low - 0xDC00U);
                    reader = lowReader;
                    if constexpr (std::same_as<std::invoke_result_t<Function, Char>, bool>) {
                        if (!function(Char{codePoint})) {
                            return false;
                        }
                    } else {
                        function(Char{codePoint});
                    }
                    unitIndex += 2U;
                    continue;
                }
            }
            if constexpr (errorMode == EncodingErrorMode::Throw) {
                reader.setPosition(unitPosition);
                err::throwU16EncodingError("Invalid UTF-16 high surrogate", unitIndex);
            } else {
                if constexpr (errorMode == EncodingErrorMode::Replace) {
                    if constexpr (std::same_as<std::invoke_result_t<Function, Char>, bool>) {
                        if (!function(Char::replacement())) {
                            return false;
                        }
                    } else {
                        function(Char::replacement());
                    }
                }
                unitIndex += 1U;
                continue;
            }
        }
        if (Char::isLowSurrogate(unit)) {
            if constexpr (errorMode == EncodingErrorMode::Throw) {
                reader.setPosition(unitPosition);
                err::throwU16EncodingError("Invalid UTF-16 low surrogate", unitIndex);
            } else {
                if constexpr (errorMode == EncodingErrorMode::Replace) {
                    if constexpr (std::same_as<std::invoke_result_t<Function, Char>, bool>) {
                        if (!function(Char::replacement())) {
                            return false;
                        }
                    } else {
                        function(Char::replacement());
                    }
                }
                unitIndex += 1U;
                continue;
            }
        }
        if constexpr (std::same_as<std::invoke_result_t<Function, Char>, bool>) {
            if (!function(Char{static_cast<char32_t>(unit)})) {
                return false;
            }
        } else {
            function(Char{static_cast<char32_t>(unit)});
        }
        unitIndex += 1U;
    }
    return true;
}

/// Iterate over UTF-16 encoded byte data from a byte reader with a runtime selected error handling mode.
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

}
