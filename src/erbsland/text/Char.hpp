// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AsciiCategory.hpp"
#include "Char_fwd.hpp"
#include "CharCompareFn.hpp"
#include "CharSignal.hpp"
#include "IntegerBase.hpp"
#include "LetterCase.hpp"
#include "StringEncoding.hpp"
#include "StringKind.hpp"
#include "UnicodeCategory.hpp"
#include "UnicodeCategoryGroup.hpp"

#include "../unit/ByteLength_fwd.hpp"
#include "../unit/U16DataLength_fwd.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <optional>

namespace erbsland::text {

/// A single 32bit Unicode code-point.
/// @seedoc{/reference/text/char_range}
/// @tested{CharTest}
class Char final {
public:
    /// Create a Char from a Unicode code-point.
    constexpr Char(const char32_t codePoint) noexcept : _codePoint{codePoint} {} // NOLINT(*-explicit-constructor)

    // defaults
    Char() = default;
    ~Char() = default;
    Char(const Char &) = default;
    Char(Char &&) = default;
    auto operator=(const Char &) -> Char & = default;
    auto operator=(Char &&) -> Char & = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_codePoint, const Char &other, other._codePoint);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_codePoint, const char32_t codePoint, codePoint);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(const char32_t codePoint, const Char &other, codePoint, other._codePoint);
    constexpr auto operator==(const CharSignal signal) const noexcept -> bool { return *this == fromSignal(signal); }
    constexpr auto operator!=(const CharSignal signal) const noexcept -> bool { return !operator==(signal); }

public: // accessors
    /// Access the raw underlying code-point.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> char32_t { return _codePoint; }

public: // tests
    /// Test if the given UTF-16 code unit is a high surrogate.
    [[nodiscard]] constexpr static auto isHighSurrogate(const char16_t value) noexcept -> bool {
        return value >= 0xD800U && value <= 0xDBFFU;
    }
    /// Test if the given UTF-16 code unit is a low surrogate.
    [[nodiscard]] constexpr static auto isLowSurrogate(const char16_t value) noexcept -> bool {
        return value >= 0xDC00U && value <= 0xDFFFU;
    }
    /// Test if the Char is in the ASCII range.
    [[nodiscard]] constexpr auto isAscii() const noexcept -> bool { return _codePoint <= 0x7FU; }
    /// Test if the Char is an ASCII letter.
    [[nodiscard]] constexpr auto isAsciiLetter() const noexcept -> bool {
        return (_codePoint >= U'A' && _codePoint <= U'Z') || (_codePoint >= U'a' && _codePoint <= U'z');
    }
    /// Test if the Char is an ASCII lowercase letter.
    [[nodiscard]] constexpr auto isAsciiLowercaseLetter() const noexcept -> bool {
        return _codePoint >= U'a' && _codePoint <= U'z';
    }
    /// Test if the Char is an ASCII uppercase letter.
    [[nodiscard]] constexpr auto isAsciiUppercaseLetter() const noexcept -> bool {
        return _codePoint >= U'A' && _codePoint <= U'Z';
    }
    /// Test if the Char is an ASCII digit.
    [[nodiscard]] constexpr auto isAsciiDigit() const noexcept -> bool {
        return _codePoint >= U'0' && _codePoint <= U'9';
    }
    /// Test if the Char is an ASCII hexadecimal digit.
    [[nodiscard]] constexpr auto isAsciiHexDigit() const noexcept -> bool {
        return (_codePoint >= U'0' && _codePoint <= U'9') || (_codePoint >= U'A' && _codePoint <= U'F') ||
            (_codePoint >= U'a' && _codePoint <= U'f');
    }
    /// Test if the Char is an ASCII alphanumeric character.
    [[nodiscard]] constexpr auto isAsciiAlphanumeric() const noexcept -> bool {
        return isAsciiLetter() || isAsciiDigit();
    }
    /// Test if the Char is an ASCII word character (letter, digit, or underscore).
    [[nodiscard]] constexpr auto isAsciiWord() const noexcept -> bool {
        return isAsciiAlphanumeric() || _codePoint == U'_';
    }
    /// Get the ASCII digit value.
    [[nodiscard]] constexpr auto digitValue() const noexcept -> std::optional<unsigned int> {
        if (_codePoint >= U'0' && _codePoint <= U'9') {
            return static_cast<unsigned int>(_codePoint - U'0');
        }
        if (_codePoint >= U'A' && _codePoint <= U'Z') {
            return static_cast<unsigned int>(_codePoint - U'A') + 10U;
        }
        if (_codePoint >= U'a' && _codePoint <= U'z') {
            return static_cast<unsigned int>(_codePoint - U'a') + 10U;
        }
        return {};
    }
    /// Get the ASCII digit value if it is valid for the given integer base.
    [[nodiscard]] constexpr auto digitValue(const IntegerBase base) const noexcept -> std::optional<unsigned int> {
        const auto value = digitValue();
        if (value.has_value() && value.value() < base.baseFactor()) {
            return value;
        }
        return {};
    }
    /// Test if this character is a valid digit in the given integer base.
    [[nodiscard]] constexpr auto isDigitValue(const IntegerBase base) const noexcept -> bool {
        return digitValue(base).has_value();
    }
    /// Test if the Char is an ASCII whitespace character.
    [[nodiscard]] constexpr auto isAsciiWhitespace() const noexcept -> bool {
        return _codePoint == U' ' || (_codePoint >= U'\t' && _codePoint <= U'\r');
    }
    /// Test if the Char is an ASCII blank character (space or tab).
    [[nodiscard]] constexpr auto isAsciiBlank() const noexcept -> bool {
        return _codePoint == U' ' || _codePoint == U'\t';
    }
    /// Test if the Char is an ASCII control character.
    [[nodiscard]] constexpr auto isAsciiControl() const noexcept -> bool {
        return _codePoint <= 0x1FU || _codePoint == 0x7FU;
    }
    /// Test if the Char is an ASCII punctuation character.
    [[nodiscard]] constexpr auto isAsciiPunctuation() const noexcept -> bool {
        return (_codePoint >= U'!' && _codePoint <= U'/') || (_codePoint >= U':' && _codePoint <= U'@') ||
            (_codePoint >= U'[' && _codePoint <= U'`') || (_codePoint >= U'{' && _codePoint <= U'~');
    }
    /// Test if the Char has a special meaning in regular-expression syntax.
    [[nodiscard]] constexpr auto isSpecialRegexCharacter() const noexcept -> bool {
        return _codePoint == U'.' || _codePoint == U'\\' || _codePoint == U'^' || _codePoint == U'$' ||
            _codePoint == U'|' || _codePoint == U'{' || _codePoint == U'}' || _codePoint == U'(' ||
            _codePoint == U')' || _codePoint == U'[' || _codePoint == U']' || _codePoint == U'+' ||
            _codePoint == U'*' || _codePoint == U'?';
    }
    /// Test if the Char matches an ASCII-only category without using the Unicode Light database.
    [[nodiscard]] constexpr auto isAsciiCategory(const AsciiCategory category) const noexcept -> bool {
        if (!isAscii()) {
            return false;
        }
        switch (category) {
        case AsciiCategory::Letter:
            return isAsciiLetter();
        case AsciiCategory::LowercaseLetter:
            return isAsciiLowercaseLetter();
        case AsciiCategory::UppercaseLetter:
            return isAsciiUppercaseLetter();
        case AsciiCategory::Digit:
            return isAsciiDigit();
        case AsciiCategory::HexDigit:
            return isAsciiHexDigit();
        case AsciiCategory::Alphanumeric:
            return isAsciiAlphanumeric();
        case AsciiCategory::Whitespace:
            return isAsciiWhitespace();
        case AsciiCategory::Blank:
            return isAsciiBlank();
        case AsciiCategory::Control:
            return isAsciiControl();
        case AsciiCategory::Punctuation:
            return isAsciiPunctuation();
        }
        return false;
    }
    /// Test if the Char represents a null character (code-point 0).
    [[nodiscard]] constexpr auto isNull() const noexcept -> bool { return _codePoint == 0; }
    /// Test if the Char is a replacement character
    [[nodiscard]] constexpr auto isReplacement() const noexcept -> bool { return _codePoint == 0xFFFD; }
    /// Test if the Char is a reserved non-character signal.
    [[nodiscard]] constexpr auto isSignal() const noexcept -> bool { return _codePoint >= 0xFFFFFF00U; }
    /// Test if the Char represents the end of data.
    [[nodiscard]] constexpr auto isEndOfData() const noexcept -> bool { return *this == CharSignal::EndOfData; }
    /// Test if the Char represents the absence of a code point.
    [[nodiscard]] constexpr auto isNoCodePoint() const noexcept -> bool { return *this == CharSignal::NoCodePoint; }
    /// Test if the Char represents an internal character-processing failure.
    [[nodiscard]] constexpr auto isError() const noexcept -> bool { return *this == CharSignal::Error; }
    /// Test if the Char represents the encoding-boundary byte-order-mark signal.
    [[nodiscard]] constexpr auto isByteOrderMark() const noexcept -> bool { return *this == CharSignal::ByteOrderMark; }
    /// Test if the Char represents a valid Unicode code-point.
    /// Erbsland Core reserves U+FEFF for encoded-data boundaries and rejects it as text content.
    [[nodiscard]] constexpr auto isValidUnicode() const noexcept -> bool {
        return _codePoint <= 0x10FFFF && _codePoint != 0xFEFFU && (_codePoint < 0xD800 || _codePoint > 0xDFFF);
    }
    /// Test if the Char can be displayed directly in diagnostics and other safe strings.
    /// This rejects invalid values, controls, invisible formatting characters, and reserved display ranges.
    [[nodiscard]] constexpr auto isSafeUnicode() const noexcept -> bool {
        return isValidUnicode() && _codePoint > 0x001FU && !(_codePoint >= 0x007FU && _codePoint <= 0x009FU) &&
            _codePoint != 0x061CU && !(_codePoint >= 0x200EU && _codePoint <= 0x200FU) &&
            !(_codePoint >= 0x202AU && _codePoint <= 0x202EU) && !(_codePoint >= 0x2066U && _codePoint <= 0x2069U) &&
            !(_codePoint >= 0x2400U && _codePoint <= 0x243FU) && !(_codePoint >= 0xFE00U && _codePoint <= 0xFE0FU) &&
            !(_codePoint >= 0xFFF9U && _codePoint <= 0xFFFBU) && !(_codePoint >= 0xE0000U && _codePoint <= 0xE007FU) &&
            !(_codePoint >= 0xE0100U && _codePoint <= 0xE01EFU);
    }
    /// Get the Unicode general category for this character.
    /// @usesunidb{Uses generated Unicode Character Database character metadata.}
    [[nodiscard]] auto category() const noexcept -> UnicodeCategory;
    /// Get the Unicode general category group for this character.
    /// @usesunidb{Uses generated Unicode Character Database character metadata.}
    [[nodiscard]] auto categoryGroup() const noexcept -> UnicodeCategoryGroup {
        return static_cast<UnicodeCategoryGroup>(static_cast<uint8_t>(category()) >> 4U);
    }
    /// Test if the character is in the given Unicode general category group.
    /// @usesunidb{Uses generated Unicode Character Database character metadata.}
    [[nodiscard]] auto isCategoryGroup(const UnicodeCategoryGroup expectedCategoryGroup) const noexcept -> bool {
        return categoryGroup() == expectedCategoryGroup;
    }
    /// Test if the character is in the given Unicode general category.
    /// @usesunidb{Uses generated Unicode Character Database character metadata.}
    [[nodiscard]] auto isCategory(const UnicodeCategory expectedCategory) const noexcept -> bool {
        return category() == expectedCategory;
    }
    /// Test if the character is a Unicode control character.
    /// @usesunidb{Uses generated Unicode Character Database character metadata.}
    [[nodiscard]] auto isControl() const noexcept -> bool;
    /// Test if the character is a Unicode control or format character.
    /// @usesunidb{Uses generated Unicode Character Database character metadata.}
    [[nodiscard]] auto isControlOrFormat() const noexcept -> bool;

public: // encoding information.
    /// Get the approximate display width for this Unicode code point.
    /// Invalid characters and Unicode control characters have width zero.
    /// @usesunidb{Uses generated Unicode Character Database character metadata.}
    [[nodiscard]] auto displayWidth() const noexcept -> int;
    /// Get the size of the character in UTF-8 bytes.
    [[nodiscard]] auto utf8Size() const noexcept -> unit::ByteLength;
    /// Get the size of the character in UTF-16 units.
    [[nodiscard]] auto utf16Size() const noexcept -> unit::U16DataLength;
    /// The encoded size in the unit of the encoding.
    /// For UTF-8, this is bytes (uint8_t).
    /// For UTF-16, this is words (uint16_t).
    /// For UTF-32, this is code points (uint32_t).
    /// @param stringKind The kind of string encoding to query the size for.
    /// @return The encoded size of the character in the specified string kind.
    [[nodiscard]] auto encodedSize(StringKind stringKind) const noexcept -> std::size_t;
    /// Get the number of bytes required to encode this character.
    /// Invalid Unicode values and character signals have an encoded length of zero.
    /// @param encoding The target byte encoding.
    [[nodiscard]] auto encodedBytes(StringEncoding encoding) const noexcept -> unit::ByteLength;

public: // conversion
    /// Return the simple one-code-point case-folded form of this character.
    /// @usesunidb{Uses generated Unicode Character Database character metadata.}
    [[nodiscard]] auto caseFolded() const noexcept -> Char;
    /// Return the simple one-code-point case-folded form of a character.
    /// @usesunidb{Uses generated Unicode Character Database character metadata.}
    [[nodiscard]] static auto caseFolded(Char character) noexcept -> Char { return character.caseFolded(); }
    /// Convert this character to its simple lowercase form.
    /// @usesunidb{Uses generated Unicode Character Database character metadata.}
    [[nodiscard]] auto toLowercase() const noexcept -> Char;
    /// Convert a character to its simple lowercase form.
    /// @usesunidb{Uses generated Unicode Character Database character metadata.}
    [[nodiscard]] static auto toLowercase(Char character) noexcept -> Char { return character.toLowercase(); }
    /// Convert this character to its simple uppercase form.
    /// @usesunidb{Uses generated Unicode Character Database character metadata.}
    [[nodiscard]] auto toUppercase() const noexcept -> Char;
    /// Convert a character to its simple uppercase form.
    /// @usesunidb{Uses generated Unicode Character Database character metadata.}
    [[nodiscard]] static auto toUppercase(Char character) noexcept -> Char { return character.toUppercase(); }
    /// Convert ASCII uppercase letters to lowercase without using the Unicode database.
    [[nodiscard]] constexpr auto toAsciiLowercase() const noexcept -> Char {
        if (isAsciiUppercaseLetter()) {
            return Char{static_cast<char32_t>(_codePoint + (U'a' - U'A'))};
        }
        return *this;
    }
    /// Convert ASCII uppercase letters in a character to lowercase without using the Unicode database.
    [[nodiscard]] constexpr static auto toAsciiLowercase(const Char character) noexcept -> Char {
        return character.toAsciiLowercase();
    }
    /// Convert ASCII lowercase letters to uppercase without using the Unicode database.
    [[nodiscard]] constexpr auto toAsciiUppercase() const noexcept -> Char {
        if (isAsciiLowercaseLetter()) {
            return Char{static_cast<char32_t>(_codePoint - (U'a' - U'A'))};
        }
        return *this;
    }
    /// Convert ASCII lowercase letters in a character to uppercase without using the Unicode database.
    [[nodiscard]] constexpr static auto toAsciiUppercase(const Char character) noexcept -> Char {
        return character.toAsciiUppercase();
    }
    /// Convert to an identifier normalized character.
    /// This is ASCII-only case-folded, and space (U+0020) is converted to an underscore (U+005F).
    [[nodiscard]] constexpr auto toIdentifierNormalized() const noexcept -> Char {
        if (*this == U' ') {
            return Char{U'_'};
        }
        return toAsciiLowercase();
    }
    /// Convert to an identifier normalized character.
    [[nodiscard]] constexpr static auto toIdentifierNormalized(const Char character) noexcept -> Char {
        return character.toIdentifierNormalized();
    }

public: // comparison methods
    /// Compare this character using ASCII-only case folding.
    [[nodiscard]] constexpr auto compareAsciiFolded(const Char other) const noexcept -> std::strong_ordering {
        return toAsciiLowercase() <=> other.toAsciiLowercase();
    }
    /// Compare two characters using ASCII-only case folding.
    [[nodiscard]] static auto compareAsciiFolded(Char left, Char right) noexcept -> std::strong_ordering;
    /// Compare this character using Unicode case folding
    [[nodiscard]] auto compareCaseFolded(Char other) const noexcept -> std::strong_ordering;
    /// Get a comparison callback that applies Unicode case folding.
    [[nodiscard]] static auto compareCaseFolded(Char left, Char right) noexcept -> std::strong_ordering;
    /// Compare this character using identifier folding.
    /// This is ASCII-only case-folded, and space (U+0020) is equal to an underscore (U+005F).
    [[nodiscard]] auto compareIdentifier(const Char other) const noexcept -> std::strong_ordering {
        return toIdentifierNormalized() <=> other.toIdentifierNormalized();
    }
    /// Compare two characters using identifier folding.
    [[nodiscard]] static auto compareIdentifier(Char left, Char right) noexcept -> std::strong_ordering;

public: // factory methods
    /// Get the null character (U+0000).
    [[nodiscard]] constexpr static auto null() noexcept -> Char { return Char{0U}; }
    /// Get the replacement character (U+FFFD).
    /// This library uses the Unicode replacement character (U+FFFD) to represent invalid or unrepresentable characters.
    [[nodiscard]] constexpr static auto replacement() noexcept -> Char { return Char{0xFFFDU}; }
    /// Create a Char from a reserved non-character signal.
    [[nodiscard]] constexpr static auto fromSignal(const CharSignal signal) noexcept -> Char {
        return Char{static_cast<char32_t>(0xFFFFFFFFU - static_cast<uint8_t>(signal))};
    }
    /// Get the end-of-data signal.
    [[nodiscard]] constexpr static auto endOfData() noexcept -> Char { return fromSignal(CharSignal::EndOfData); }
    /// Get the no-code-point signal.
    [[nodiscard]] constexpr static auto noCodePoint() noexcept -> Char { return fromSignal(CharSignal::NoCodePoint); }
    /// Get the internal character-processing error signal.
    /// This signal is reserved for internal algorithms and is never returned by public Erbsland Core text APIs.
    [[nodiscard]] constexpr static auto error() noexcept -> Char { return fromSignal(CharSignal::Error); }
    /// Get the encoding-independent byte-order-mark signal.
    /// This signal is reserved for encoded-data boundaries and is never exposed as text content.
    [[nodiscard]] constexpr static auto byteOrderMark() noexcept -> Char {
        return fromSignal(CharSignal::ByteOrderMark);
    }
    /// Create an ASCII digit character from a value.
    [[nodiscard]] constexpr static auto fromDigitValue(
        const unsigned int digit, const LetterCase letterCase = LetterCase::Lowercase) noexcept -> Char {
        if (digit < 10U) {
            return Char{static_cast<char32_t>(U'0' + digit)};
        }
        const auto firstLetter = letterCase == LetterCase::Uppercase ? U'A' : U'a';
        return Char{static_cast<char32_t>(firstLetter + (digit - 10U))};
    }

private:
    [[nodiscard]] static auto applyDelta(char32_t codePoint, int32_t delta) noexcept -> Char;

protected:
    char32_t _codePoint{0}; ///< The Unicode code-point.
};

}
