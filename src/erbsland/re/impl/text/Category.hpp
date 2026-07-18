// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Character.hpp"

#include "../../../text/CharSet.hpp"
#include "../../../text/String.hpp"
#include "../../../text/StringHashMap.hpp"
#include "../../../util/impl/ComparisonHelper.hpp"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace erbsland::re::impl {

namespace ch {
/// The mask type for the category.
using Mask = uint32_t;

constexpr static std::size_t cUnicodeSubCategoryBitCount = 7U;
constexpr static std::size_t cUnicodeCategoryBitCount = 7U;
constexpr static std::size_t cReSubcategoryBitCount = 1U;
constexpr static std::size_t cReCategoryBitCount = 8U;
constexpr static std::size_t cTotalBits =
    cUnicodeSubCategoryBitCount + cUnicodeCategoryBitCount + cReSubcategoryBitCount + cReCategoryBitCount;
static_assert(cTotalBits <= 24U, "The total number of bits for category masks exceeds the maximum allowed (24 bits).");

constexpr static std::size_t cUnicodeSubCategoryBaseBit = 0;
constexpr static std::size_t cUnicodeCategoryBaseBit = cUnicodeSubCategoryBaseBit + cUnicodeSubCategoryBitCount;
constexpr static std::size_t cReSubCategoryBaseBit = cUnicodeCategoryBaseBit + cUnicodeCategoryBitCount;
constexpr static std::size_t cReCategoryBaseBit = cReSubCategoryBaseBit + cReSubcategoryBitCount;

constexpr static auto ucValue(const std::size_t categoryIndex) noexcept -> Mask {
    return static_cast<Mask>(1U) << (categoryIndex + cUnicodeCategoryBaseBit);
}
constexpr static auto ucValue(const std::size_t categoryIndex, const std::size_t subcategoryIndex) noexcept -> Mask {
    return ucValue(categoryIndex) | static_cast<Mask>(1U) << (subcategoryIndex + cUnicodeSubCategoryBaseBit);
}
constexpr static auto reValue(const std::size_t categoryIndex, bool isAscii) noexcept -> Mask {
    return static_cast<Mask>(1U) << (categoryIndex + cReCategoryBaseBit) |
        static_cast<Mask>(isAscii ? 1U : 0U) << cReSubCategoryBaseBit;
}
constexpr static auto reAsciiValue(const std::size_t categoryIndex) noexcept -> Mask {
    return reValue(categoryIndex, true); // Ascii is more limited than Unicode, ASCII flag must be set.
}
constexpr static auto reUnicodeValue(const std::size_t categoryIndex) noexcept -> Mask {
    return reValue(categoryIndex, false); // Unicode variant includes all ASCII characters.
}

}

/// A named character category.
/// All Unicode categories and extensions.
/// @tested{CategoryContainsTest}
class Category {
public:
    using Mask = ch::Mask;
    constexpr static auto cNotComputed = static_cast<Mask>(0U);

    /// The actual enum, but also the tested bitmask.
    /// Bitmask structure:
    /// Bit 0-(cCategoryBaseBit-1): Subcategory
    /// Bit cCategoryBaseBit-...: Category
    enum Value : Mask {
        None = 0, // For error handling.

        // Unicode 1-letter classes
        Other = ch::ucValue(0U),
        C = Other,
        Letter = ch::ucValue(1U),
        L = Letter,
        Mark = ch::ucValue(2U),
        M = Mark,
        Number = ch::ucValue(3U),
        N = Number,
        Punctuation = ch::ucValue(4U),
        P = Punctuation,
        Symbol = ch::ucValue(5U),
        S = Symbol,
        Separator = ch::ucValue(6U),
        Z = Separator,
        // Unicode 2-letter classes
        Control = ch::ucValue(0U, 0U),
        Cc = Control,
        Format = ch::ucValue(0U, 1U),
        Cf = Format,
        Unassigned = ch::ucValue(0U, 2U),
        Cn = Unassigned,
        PrivateUse = ch::ucValue(0U, 3U),
        Co = PrivateUse,
        Surrogate = ch::ucValue(0U, 4U),
        Cs = Surrogate,
        LowercaseLetter = ch::ucValue(1U, 0U),
        Ll = LowercaseLetter,
        ModifierLetter = ch::ucValue(1U, 1U),
        Lm = ModifierLetter,
        OtherLetter = ch::ucValue(1U, 2U),
        Lo = OtherLetter,
        TitlecaseLetter = ch::ucValue(1U, 3U),
        Lt = TitlecaseLetter,
        UppercaseLetter = ch::ucValue(1U, 4U),
        Lu = UppercaseLetter,
        SpacingMark = ch::ucValue(2U, 0U),
        Mc = SpacingMark,
        EnclosingMark = ch::ucValue(2U, 1U),
        Me = EnclosingMark,
        NonspacingMark = ch::ucValue(2U, 2U),
        Mn = NonspacingMark,
        DecimalNumber = ch::ucValue(3U, 0U),
        Nd = DecimalNumber,
        LetterNumber = ch::ucValue(3U, 1U),
        Nl = LetterNumber,
        OtherNumber = ch::ucValue(3U, 2U),
        No = OtherNumber,
        ConnectorPunctuation = ch::ucValue(4U, 0U),
        Pc = ConnectorPunctuation,
        DashPunctuation = ch::ucValue(4U, 1U),
        Pd = DashPunctuation,
        ClosePunctuation = ch::ucValue(4U, 2U),
        Pe = ClosePunctuation,
        FinalPunctuation = ch::ucValue(4U, 3U),
        Pf = FinalPunctuation,
        InitialPunctuation = ch::ucValue(4U, 4U),
        Pi = InitialPunctuation,
        OtherPunctuation = ch::ucValue(4U, 5U),
        Po = OtherPunctuation,
        OpenPunctuation = ch::ucValue(4U, 6U),
        Ps = OpenPunctuation,
        CurrencySymbol = ch::ucValue(5U, 0U),
        Sc = CurrencySymbol,
        ModifierSymbol = ch::ucValue(5U, 1U),
        Sk = ModifierSymbol,
        MathSymbol = ch::ucValue(5U, 2U),
        Sm = MathSymbol,
        OtherSymbol = ch::ucValue(5U, 3U),
        So = OtherSymbol,
        LineSeparator = ch::ucValue(6U, 0U),
        Zl = LineSeparator,
        ParagraphSeparator = ch::ucValue(6U, 1U),
        Zp = ParagraphSeparator,
        SpaceSeparator = ch::ucValue(6U, 2U),
        Zs = SpaceSeparator,
        // Special RegExp categories for \d, \w, \s, \h, \v, .
        DigitUnicode = ch::reUnicodeValue(0U),
        DigitAscii = ch::reAsciiValue(0U),
        WordUnicode = ch::reUnicodeValue(1U),
        WordAscii = ch::reAsciiValue(1U),
        // We include the tab character in all `Space...` categories.
        SpaceUnicode = ch::reUnicodeValue(2U),
        SpaceAscii = ch::reAsciiValue(2U),
        SpaceUnicodeDotAll = ch::reUnicodeValue(3U),
        SpaceAsciiDotAll = ch::reAsciiValue(3U),
        // Horizontal and vertical space is roughly modeled after PCRE.
        HorizontalSpaceUnicode = ch::reUnicodeValue(4U),
        HorizontalSpaceAscii = ch::reAsciiValue(4U),
        VerticalSpaceUnicode = ch::reUnicodeValue(5U),
        VerticalSpaceAscii = ch::reAsciiValue(5U),
        // Any and AnyDotAll will compiled into `NOT CHAR '\n'` and `ANY`.
        // They will never be used as a category.
        Any = ch::reUnicodeValue(6U),
        AnyDotAll = ch::reUnicodeValue(7U),
    };
    friend constexpr auto operator|(const Value a, const Value b) noexcept -> Value {
        return static_cast<Value>(static_cast<Mask>(a) | static_cast<Mask>(b));
    }
    static constexpr std::size_t maximumNameLength = 20U;

public:
    /// Create a new category from the given enum value.
    constexpr Category(const Value value) : _value{value} {} // NOLINT(*-explicit-constructor)

    // defaults
    Category() = default;
    ~Category() = default;
    Category(const Category &) = default;
    auto operator=(const Category &) -> Category & = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const Category &other, other._value);

public: // test
    /// This if this category includes another category.
    [[nodiscard]] auto includes(const Category &other) const noexcept -> bool;
    /// Test if a character is contained in this character category.
    [[nodiscard]] auto contains(text::Char character) const noexcept -> bool;
    /// Compute the complete regular-expression category mask for a Unicode scalar value.
    /// Invalid Unicode values and signals return an empty mask.
    [[nodiscard]] static auto maskFor(text::Char character) noexcept -> Mask;

public: // conversion
    /// Get the character class for a given name.
    /// Assumes a lowercase name with removed underscores.
    /// @return The character class or `None` if it wasn't found.
    [[nodiscard]] static auto fromString(const text::String &str) -> Category;

    /// Get a character class for a given unprocessed name.
    /// @param str The unprocessed string.
    /// @return The category.
    /// @throws std::out_of_bounds if the category is unknown.
    [[nodiscard]] static auto fromUnprocessedString(const text::String &str) -> Category;

    /// Return a long string for the given character category.
    [[nodiscard]] auto toLongString() const -> text::String;

    /// Return a short string for the given character category.
    [[nodiscard]] auto toShortString() const -> text::String;

    /// Create the Core character set represented by this category.
    [[nodiscard]] auto characterSet() const -> text::CharSet;

    /// Return the raw value.
    [[nodiscard]] constexpr auto raw() const noexcept -> Value { return _value; }

    /// Return the mask.
    [[nodiscard]] constexpr auto mask() const noexcept -> Mask { return static_cast<Mask>(_value); }

public: // helper methods.
    /// Normalize a list of categories.
    /// - Sort the list.
    /// - Remove duplicated.
    /// - Remove subcategories that are covered by other categories.
    /// @param list The list of categories to normalize.
    static void normalizeList(std::vector<Category> &list) noexcept;

private:
    [[nodiscard]] static constexpr auto unicodeMaskFor(text::UnicodeCategory category) noexcept -> Mask;
    static constexpr void addRegexMask(
        Mask &mask, bool unicodeMatches, bool asciiMatches, Value unicodeValue, Value asciiValue) noexcept;

private:
    using NameToValueMap = text::StringHashMap<Value>;
    struct Names {
        text::String unicodeShort;
        text::String unicodeLong;
    };
    using ValueToNameMap = std::unordered_map<Value, Names>;
    [[nodiscard]] static auto nameToValueMap() noexcept -> const NameToValueMap &;
    [[nodiscard]] static auto valueToNameMap() noexcept -> const ValueToNameMap &;

private:
    Value _value = None;
};

}
