// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Char.hpp"
#include "EncodingErrorMode.hpp"
#include "String.hpp"

#include "impl/UnicodeData.hpp"
#include "u32/U32String.hpp"
#include "u32/U32StringView.hpp"

#include "../unit/ByteLength.hpp"
#include "../unit/CpLength.hpp"

#include <array>
#include <cstddef>

namespace erbsland::text {

/// Representation of one Unicode character with optional combining marks.
/// @seedoc{/reference/text/char_range}
/// @tested{CombinedCharTest}
class CombinedChar final {
public:
    using Storage = std::array<Char, 3>; ///< Fixed storage for one base character and up to two combining marks.

public:
    /// Construct an empty character.
    constexpr CombinedChar() noexcept = default;
    /// Construct a character from a single code point.
    /// @param codePoint The Unicode code point to store as the base character.
    constexpr explicit CombinedChar(const Char codePoint) noexcept : _characters{codePoint, {}, {}} {}
    /// Construct a character from UTF-8 text.
    /// @param text The UTF-8 text for exactly one terminal character.
    /// Invalid UTF-8 bytes are replaced per byte, and unsupported text normalizes to `U+FFFD`.
    /// Empty input, control codes, and leading zero-width code points normalize to `U+FFFD`.
    /// Later visible code points also collapse the result to `U+FFFD`, while a third combining mark is ignored.
    explicit CombinedChar(const StringView &text) noexcept;
    /// Construct a character from UTF-32 text.
    /// @param text The UTF-32 text for exactly one terminal character.
    /// Invalid Unicode scalar values and unsupported text normalize to `U+FFFD`.
    /// Empty input, control codes, and leading zero-width code points normalize to `U+FFFD`.
    /// Later visible code points also collapse the result to `U+FFFD`, while a third combining mark is ignored.
    explicit CombinedChar(const U32StringView &text) noexcept;

    // defaults
    ~CombinedChar() = default;
    CombinedChar(const CombinedChar &) = default;
    CombinedChar(CombinedChar &&) = default;
    auto operator=(const CombinedChar &) -> CombinedChar & = default;
    auto operator=(CombinedChar &&) -> CombinedChar & = default;

public: // operators
    /// Compare two stored character sequences.
    auto operator==(const CombinedChar &other) const noexcept -> bool { return _characters == other._characters; }
    /// Compare two stored character sequences.
    auto operator!=(const CombinedChar &other) const noexcept -> bool { return _characters != other._characters; }
    /// Compare against a single code point.
    [[nodiscard]] auto operator==(Char other) const noexcept -> bool { return singleOrNull() == other; }
    /// Compare against a single code point.
    [[nodiscard]] auto operator!=(Char other) const noexcept -> bool { return !operator==(other); }

public: // accessors
    /// Convert the stored sequence to UTF-8.
    [[nodiscard]] auto toString() const -> String;
    /// Convert the stored sequence to UTF-32.
    [[nodiscard]] auto toU32String() const -> U32String;
    /// Get the leading code point.
    [[nodiscard]] constexpr auto first() const noexcept -> Char { return _characters[0]; }
    /// Get a single Unicode code point or zero for combined or empty characters.
    /// This is a fast-path method for comparing a single-code point character, without the color.
    /// @return The single code point, or `0` if this character is combined or empty.
    [[nodiscard]] constexpr auto singleOrNull() const noexcept -> Char {
        return !_characters[1].isNull() ? Char{} : _characters[0];
    }
    /// Get the raw stored characters.
    [[nodiscard]] constexpr auto characters() const noexcept -> const Storage & { return _characters; }
    /// Get the number of stored code points.
    [[nodiscard]] constexpr auto characterCount() const noexcept -> unit::CpLength {
        return unit::CpLength::fromSizeT(countCodePoints(_characters));
    }
    /// Get the display width of the leading character.
    /// @usesunidb{Uses generated Unicode Character Database character metadata.}
    [[nodiscard]] auto displayWidth() const noexcept -> int { return first().displayWidth(); }
    /// Get the UTF-8 byte count for this sequence.
    [[nodiscard]] auto byteCount() const noexcept -> unit::ByteLength;

public: // modifiers
    /// Create a copy with one additional combining code point.
    /// @param codePoint The combining code point to append.
    /// @param encodingErrors How invalid combining code points are handled.
    /// @return The updated character.
    /// `EncodingErrorMode::Replace` and `EncodingErrorMode::Ignore` keep the original character unchanged for invalid
    /// combining code points or when the storage is already full.
    /// @throws text::EncodingError If `encodingErrors` is `EncodingErrorMode::Throw` and the code point is
    /// unsupported.
    [[nodiscard]] auto withCombining(
        Char codePoint, EncodingErrorMode encodingErrors = EncodingErrorMode::Replace) const -> CombinedChar;

public: // tests
    /// Test if the character is empty.
    [[nodiscard]] constexpr auto isEmpty() const noexcept -> bool { return _characters[0] == 0; }
    /// Test if the character is spacing.
    [[nodiscard]] auto isSpacing() const noexcept -> bool {
        if (characterCount() != unit::CpLength::one()) {
            return false;
        }
        return _characters[0] == U' ' || _characters[0] == U'\t' || _characters[0] == U'\n' || _characters[0] == U'\r';
    }
    /// Test if the leading code point is a control code.
    /// @usesunidb{Uses generated Unicode Character Database character metadata.}
    [[nodiscard]] auto isControl() const noexcept -> bool { return first().isControl(); }
    /// Get a stable hash for the stored code points.
    [[nodiscard]] constexpr auto hash() const noexcept -> std::size_t {
        return hashCreate(_characters[0].toRawValue(), _characters[1].toRawValue(), _characters[2].toRawValue());
    }

public: // conversion
    /// Parse exactly one visible text character from UTF-8 input.
    /// Invalid or unsupported input normalizes to `U+FFFD`.
    /// @param text The UTF-8 text to parse.
    /// @return The parsed character.
    [[nodiscard]] static auto fromString(const StringView &text) noexcept -> CombinedChar;
    /// Parse exactly one visible text character from UTF-32 input.
    /// Invalid or unsupported input normalizes to `U+FFFD`.
    /// @param text The UTF-32 text to parse.
    /// @return The parsed character.
    [[nodiscard]] static auto fromString(const U32StringView &text) noexcept -> CombinedChar;

private:
    [[nodiscard]] static auto decodeUtf8(const StringView &text) noexcept -> Storage;
    [[nodiscard]] static auto decodeUtf32(const U32StringView &text) noexcept -> Storage;
    [[nodiscard]] static auto normalizeTextCodePoint(Char codePoint) noexcept -> Char;
    [[nodiscard]] static auto normalizeDecodedText(const U32StringView &text) noexcept -> Storage;
    [[nodiscard]] static auto replacementStorage() noexcept -> Storage;
    static void normalizeDecodedTextCodePoint(
        Storage &result,
        std::size_t &combiningCount,
        bool &hasBaseCodePoint,
        bool &mustReplace,
        Char codePoint) noexcept;
    [[nodiscard]] constexpr static auto countCodePoints(const Storage &codePoints) noexcept -> std::size_t;
    [[nodiscard]] constexpr static auto hashCreate(char32_t first, char32_t second, char32_t third) noexcept
        -> std::size_t;

private:
    Storage _characters{}; ///< The stored base and combining code points.
};

constexpr auto CombinedChar::countCodePoints(const Storage &codePoints) noexcept -> std::size_t {
    for (std::size_t index = 0; index < codePoints.size(); ++index) {
        if (codePoints[index].isNull()) {
            return index;
        }
    }
    return codePoints.size();
}

constexpr auto CombinedChar::hashCreate(const char32_t first, const char32_t second, const char32_t third) noexcept
    -> std::size_t {
    auto hash = std::size_t{0};
    const auto combine = [&hash](const char32_t value) constexpr noexcept -> void {
        hash ^= static_cast<std::size_t>(value) + 0x9e3779b9U + (hash << 6U) + (hash >> 2U);
    };
    combine(first);
    combine(second);
    combine(third);
    return hash;
}

}

template <>
struct std::hash<erbsland::text::CombinedChar> {
    auto operator()(const erbsland::text::CombinedChar &character) const noexcept -> std::size_t {
        return character.hash();
    }
};
