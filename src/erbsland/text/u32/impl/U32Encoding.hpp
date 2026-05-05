// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../err/ThrowHelper.hpp"
#include "../../../mem/ByteBlockView.hpp"
#include "../../../mem/ByteReader.hpp"
#include "../../../unit/CpIndex.hpp"
#include "../../../unit/CpLength.hpp"
#include "../../StringEncoding.hpp"
#include "../../u8/impl/U8Encoding.hpp"

#include <concepts>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>

namespace erbsland::text::impl::utf32 {

/// Test if the string encoding is a UTF-32 encoding.
/// @tested{U8StringEncodingTest}
[[nodiscard]] constexpr auto isEncoding(const StringEncoding encoding) noexcept -> bool {
    return encoding == StringEncoding::Utf32 || encoding == StringEncoding::Utf32LittleEndian ||
        encoding == StringEncoding::Utf32BigEndian;
}

/// Get the UTF-32 byte order mark length.
/// @tested{U8StringEncodingTest}
[[nodiscard]] constexpr auto bomLength() noexcept -> std::size_t {
    return 4U;
}

/// Test if the byte data starts with a UTF-32 little-endian byte order mark.
/// @tested{U8StringEncodingTest}
[[nodiscard]] inline auto hasLittleEndianBom(const mem::ByteBlockView &data) noexcept -> bool {
    return data.startsWith({0xFFU, 0xFEU, 0x00U, 0x00U});
}

/// Test if the byte data starts with a UTF-32 big-endian byte order mark.
/// @tested{U8StringEncodingTest}
[[nodiscard]] inline auto hasBigEndianBom(const mem::ByteBlockView &data) noexcept -> bool {
    return data.startsWith({0x00U, 0x00U, 0xFEU, 0xFFU});
}

/// Read one UTF-32 code unit from a byte reader.
/// @tested{U8StringEncodingTest}
[[nodiscard]] inline auto readCodeUnit(mem::ByteReader &reader) noexcept -> char32_t {
    return static_cast<char32_t>(reader.readUInt32());
}

/// Get the encoded length of one valid UTF-32 character.
/// @tested{U32EncodingTest}
[[nodiscard]] constexpr auto encodedLength(const Char character) noexcept -> unit::CpLength {
    return character.isValidUnicode() ? unit::CpLength::one() : unit::CpLength::zero();
}

/// Test if all code points in the span are valid Unicode scalar values.
/// @tested{U32EncodingTest}
[[nodiscard]] inline auto isValid(const std::span<const char32_t> text) noexcept -> bool {
    for (const auto codePoint : text) {
        if (!Char{codePoint}.isValidUnicode()) {
            return false;
        }
    }
    return true;
}

/// Decode one UTF-32 character and replace invalid code points.
/// @tested{U32EncodingTest}
[[nodiscard]] inline auto decodeCharOrReplace(const std::span<const char32_t> text, unit::CpIndex &position) noexcept
    -> Char {
    if (position.isNoIndex() || position.toSizeT() >= text.size()) {
        return Char::replacement();
    }
    const auto character = Char{text[position.toSizeT()]};
    ++position;
    return character.isValidUnicode() ? character : Char::replacement();
}

/// Decode one UTF-32 character and throw for invalid code points.
/// @tested{U32EncodingTest}
[[nodiscard]] inline auto decodeCharOrThrow(const std::span<const char32_t> text, unit::CpIndex &position) -> Char {
    if (position.isNoIndex() || position.toSizeT() >= text.size()) {
        err::throwOutOfRange("Read position out of range");
    }
    const auto start = position;
    const auto character = Char{text[position.toSizeT()]};
    ++position;
    if (!character.isValidUnicode()) {
        err::throwU32EncodingError("Invalid Unicode code point", start.toSizeT());
    }
    return character;
}

/// Advance a UTF-32 index by one code point.
/// @tested{U32EncodingTest}
inline void fastAdvanceChar(const std::span<const char32_t> text, unit::CpIndex &position) noexcept {
    if (!position.isNoIndex() && position.toSizeT() < text.size()) {
        ++position;
    }
}

/// Retreat a UTF-32 index by one code point.
/// @tested{U32EncodingTest}
inline void fastRetreatChar(const std::span<const char32_t> /*text*/, unit::CpIndex &position) noexcept {
    if (!position.isNoIndex() && !position.isZero()) {
        --position;
    }
}

/// Iterate over all characters in a UTF-32 string, validating each character and handling errors according to the
/// specified mode.
/// @tested{U32EncodingTest}
template <typename tFunction>
void forEachValidatedCharacter(const std::u32string_view text, const EncodingErrorMode errorMode, tFunction function) {
    for (auto index = std::size_t{0}; index < text.size(); ++index) {
        const auto character = Char{text[index]};
        if (character.isValidUnicode()) {
            function(character);
            continue;
        }
        switch (errorMode) {
        case EncodingErrorMode::Throw:
            err::throwU32EncodingError("Invalid Unicode code point", index);
        case EncodingErrorMode::Ignore:
            break;
        case EncodingErrorMode::Replace:
            function(Char::replacement());
            break;
        }
    }
}

/// Iterate over all characters in a UTF-32 span, validating each character.
/// @tested{U32EncodingTest}
template <typename Function>
auto forEachDecodedCharacter(const std::span<const char32_t> text, const EncodingErrorMode errorMode, Function function)
    -> bool {
    for (auto position = std::size_t{0}; position < text.size(); ++position) {
        const auto character = Char{text[position]};
        if (character.isValidUnicode()) {
            if constexpr (std::same_as<std::invoke_result_t<Function, Char>, bool>) {
                if (!function(character)) {
                    return false;
                }
            } else {
                function(character);
            }
            continue;
        }
        switch (errorMode) {
        case EncodingErrorMode::Throw:
            err::throwU32EncodingError("Invalid Unicode code point", position);
        case EncodingErrorMode::Ignore:
            break;
        case EncodingErrorMode::Replace:
            if constexpr (std::same_as<std::invoke_result_t<Function, Char>, bool>) {
                if (!function(Char::replacement())) {
                    return false;
                }
            } else {
                function(Char::replacement());
            }
            break;
        }
    }
    return true;
}

/// Iterate over UTF-32 encoded byte data from a byte reader, validating each character.
/// @tested{U8StringEncodingTest}
template <EncodingErrorMode errorMode, typename Function>
auto forEachValidatedCharacter(mem::ByteReader &reader, Function function) -> bool {
    auto cpIndex = std::size_t{0};
    while (!reader.isAtEnd()) {
        if (!reader.canRead(4U)) {
            if constexpr (errorMode == EncodingErrorMode::Throw) {
                err::throwEncodingError("Truncated UTF-32 data");
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
                reader.advance(4U);
                continue;
            }
        }

        const auto codePointPosition = reader.position();
        const auto character = Char{readCodeUnit(reader)};
        if (character.isValidUnicode()) {
            if constexpr (std::same_as<std::invoke_result_t<Function, Char>, bool>) {
                if (!function(character)) {
                    return false;
                }
            } else {
                function(character);
            }
            cpIndex += 1U;
            continue;
        }
        if constexpr (errorMode == EncodingErrorMode::Throw) {
            reader.setPosition(codePointPosition);
            err::throwU32EncodingError("Invalid Unicode code point", cpIndex);
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
            cpIndex += 1U;
        }
    }
    return true;
}

/// Iterate over UTF-32 encoded byte data from a byte reader with a runtime selected error handling mode.
/// @tested{U8StringEncodingTest}
template <typename Function>
auto forEachValidatedCharacter(mem::ByteReader &reader, const EncodingErrorMode errorMode, Function function) -> bool {
    switch (errorMode) {
    case EncodingErrorMode::Throw:
        return forEachValidatedCharacter<EncodingErrorMode::Throw>(reader, function);
    case EncodingErrorMode::Ignore:
        return forEachValidatedCharacter<EncodingErrorMode::Ignore>(reader, function);
    case EncodingErrorMode::Replace:
        return forEachValidatedCharacter<EncodingErrorMode::Replace>(reader, function);
    }
    return false;
}

}
