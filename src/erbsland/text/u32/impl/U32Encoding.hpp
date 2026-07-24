// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../mem/ByteBlock.hpp"
#include "../../../mem/ByteReader.hpp"
#include "../../../unit/CpIndex.hpp"
#include "../../../unit/CpLength.hpp"
#include "../../impl/ThrowHelper.hpp"
#include "../../StringEncoding.hpp"
#include "../../u8/impl/U8Encoding.hpp"

#include <concepts>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>

namespace erbsland::text::impl::utf32 {

/// Read one UTF-32 code unit from a byte reader.
[[nodiscard]] inline auto readCodeUnit(mem::ByteReader &reader) noexcept -> char32_t {
    return static_cast<char32_t>(reader.readUInt32());
}

/// Get the encoded length of one valid UTF-32 character.
[[nodiscard]] constexpr auto encodedLength(const Char character) noexcept -> unit::CpLength {
    return character.isValidUnicode() ? unit::CpLength::one() : unit::CpLength::zero();
}

/// Test if all code points in the span are valid Unicode scalar values.
[[nodiscard]] inline auto isValid(const std::span<const char32_t> text) noexcept -> bool {
    for (const auto codePoint : text) {
        if (!Char{codePoint}.isValidUnicode()) {
            return false;
        }
    }
    return true;
}

/// Decode one UTF-32 character and replace invalid code points.
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
[[nodiscard]] inline auto decodeCharOrThrow(const std::span<const char32_t> text, unit::CpIndex &position) -> Char {
    if (position.isNoIndex() || position.toSizeT() >= text.size()) {
        throwOutOfRange("Read position out of range");
    }
    const auto start = position;
    const auto character = Char{text[position.toSizeT()]};
    ++position;
    if (!character.isValidUnicode()) {
        throwU32EncodingError("Invalid Unicode code point", start.toSizeT());
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
void forEachValidatedCharacter(const std::u32string_view text, const EncodingMode mode, tFunction function) {
    for (auto index = std::size_t{0}; index < text.size(); ++index) {
        const auto character = Char{text[index]};
        if (character.isValidUnicode()) {
            function(character);
            continue;
        }
        if (mode == EncodingMode::Strict) {
            throwU32EncodingError("Invalid Unicode code point", index);
        } else {
            function(Char::replacement());
        }
    }
}

/// Iterate over all characters in a UTF-32 span, validating each character.
/// @tested{U32EncodingTest}
template <typename Function>
auto forEachDecodedCharacter(const std::span<const char32_t> text, const EncodingMode mode, Function function) -> bool {
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
        if (mode == EncodingMode::Strict) {
            throwU32EncodingError("Invalid Unicode code point", position);
        } else {
            if constexpr (std::same_as<std::invoke_result_t<Function, Char>, bool>) {
                if (!function(Char::replacement())) {
                    return false;
                }
            } else {
                function(Char::replacement());
            }
        }
    }
    return true;
}

/// Iterate over UTF-32 encoded byte data from a byte reader, validating each character.
/// @tested{U8StringEncodingTest}
template <EncodingMode mode, typename Function>
auto forEachValidatedCharacter(mem::ByteReader &reader, Function function) -> bool {
    auto cpIndex = std::size_t{0};
    while (!reader.isAtEnd()) {
        if (!reader.canRead(4U)) {
            if constexpr (mode == EncodingMode::Strict) {
                throwEncodingError("Truncated UTF-32 data");
            } else {
                if constexpr (std::same_as<std::invoke_result_t<Function, Char>, bool>) {
                    if (!function(Char::replacement())) {
                        return false;
                    }
                } else {
                    function(Char::replacement());
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
        if constexpr (mode == EncodingMode::Strict) {
            reader.setPosition(codePointPosition);
            throwU32EncodingError("Invalid Unicode code point", cpIndex);
        } else {
            if constexpr (std::same_as<std::invoke_result_t<Function, Char>, bool>) {
                if (!function(Char::replacement())) {
                    return false;
                }
            } else {
                function(Char::replacement());
            }
            cpIndex += 1U;
        }
    }
    return true;
}

/// Iterate over UTF-32 encoded byte data from a byte reader with a runtime selected error handling mode.
/// @tested{U8StringEncodingTest}
template <typename Function>
auto forEachValidatedCharacter(mem::ByteReader &reader, const EncodingMode mode, Function function) -> bool {
    switch (mode) {
    case EncodingMode::Strict:
        return forEachValidatedCharacter<EncodingMode::Strict>(reader, function);
    case EncodingMode::Tolerant:
        return forEachValidatedCharacter<EncodingMode::Tolerant>(reader, function);
    }
    return false;
}

}
