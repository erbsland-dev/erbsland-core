// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Category.hpp"

#include "../../../err/ParameterFailure.hpp"
#include "../../../text/Literals.hpp"

#include <algorithm>
#include <array>
#include <ranges>

namespace erbsland::re::impl {

using namespace text::literals;
using namespace text;

auto Category::includes(const Category &other) const noexcept -> bool {
    return (mask() & other.mask()) == mask();
}

constexpr auto Category::unicodeMaskFor(const UnicodeCategory category) noexcept -> Mask {
    using UC = UnicodeCategory;
    switch (category) {
    case UC::Control:
        return Control;
    case UC::Format:
        return Format;
    case UC::Unassigned:
        return Unassigned;
    case UC::PrivateUse:
        return PrivateUse;
    case UC::Surrogate:
        return Surrogate;
    case UC::LowercaseLetter:
        return LowercaseLetter;
    case UC::ModifierLetter:
        return ModifierLetter;
    case UC::OtherLetter:
        return OtherLetter;
    case UC::TitlecaseLetter:
        return TitlecaseLetter;
    case UC::UppercaseLetter:
        return UppercaseLetter;
    case UC::SpacingMark:
        return SpacingMark;
    case UC::EnclosingMark:
        return EnclosingMark;
    case UC::NonspacingMark:
        return NonspacingMark;
    case UC::DecimalNumber:
        return DecimalNumber;
    case UC::LetterNumber:
        return LetterNumber;
    case UC::OtherNumber:
        return OtherNumber;
    case UC::ConnectorPunctuation:
        return ConnectorPunctuation;
    case UC::DashPunctuation:
        return DashPunctuation;
    case UC::ClosePunctuation:
        return ClosePunctuation;
    case UC::FinalPunctuation:
        return FinalPunctuation;
    case UC::InitialPunctuation:
        return InitialPunctuation;
    case UC::OtherPunctuation:
        return OtherPunctuation;
    case UC::OpenPunctuation:
        return OpenPunctuation;
    case UC::CurrencySymbol:
        return CurrencySymbol;
    case UC::ModifierSymbol:
        return ModifierSymbol;
    case UC::MathSymbol:
        return MathSymbol;
    case UC::OtherSymbol:
        return OtherSymbol;
    case UC::LineSeparator:
        return LineSeparator;
    case UC::ParagraphSeparator:
        return ParagraphSeparator;
    case UC::SpaceSeparator:
        return SpaceSeparator;
    }
    return 0U;
}

constexpr void Category::addRegexMask(
    Mask &mask,
    const bool unicodeMatches,
    const bool asciiMatches,
    const Value unicodeValue,
    const Value asciiValue) noexcept {
    if (unicodeMatches) {
        mask |= static_cast<Mask>(unicodeValue);
    }
    if (asciiMatches) {
        mask |= static_cast<Mask>(asciiValue);
    }
}

auto Category::computeMaskFor(const Char character) noexcept -> Mask {
    if (!character.isValidUnicode()) {
        return 0U;
    }

    const auto unicodeCategory = character.category();
    const auto unicodeGroup = static_cast<UnicodeCategoryGroup>(static_cast<std::uint8_t>(unicodeCategory) >> 4U);
    const auto value = character.toRawValue();
    auto result = unicodeMaskFor(unicodeCategory);

    const auto isDecimalNumber = unicodeCategory == UnicodeCategory::DecimalNumber;
    addRegexMask(result, isDecimalNumber, character.isAsciiDigit(), DigitUnicode, DigitAscii);

    const auto isWordUnicode = unicodeGroup == UnicodeCategoryGroup::Letter || isDecimalNumber || value == U'_';
    const auto isWordAscii = character.isAsciiWord();
    addRegexMask(result, isWordUnicode, isWordAscii, WordUnicode, WordAscii);

    const auto isSpaceSeparator = unicodeCategory == UnicodeCategory::SpaceSeparator;
    const auto isHorizontalUnicode = value == U'\t' || isSpaceSeparator;
    // U+00A0 retains the imported engine's compatibility behavior. Its ASCII subcategory bit is shared with the
    // other regular-expression categories, so it also satisfies SpaceAscii below.
    const auto isHorizontalAscii = value == U'\t' || value == U' ' || value == 0x00A0U;
    addRegexMask(result, isHorizontalUnicode, isHorizontalAscii, HorizontalSpaceUnicode, HorizontalSpaceAscii);

    const auto isVerticalAscii = value == U'\n' || value == U'\v' || value == U'\f' || value == U'\r';
    const auto isVerticalUnicode = isVerticalAscii || value == 0x0085U ||
        unicodeCategory == UnicodeCategory::LineSeparator || unicodeCategory == UnicodeCategory::ParagraphSeparator;
    addRegexMask(result, isVerticalUnicode, isVerticalAscii, VerticalSpaceUnicode, VerticalSpaceAscii);

    const auto isSpaceUnicode = value == U'\t' || isSpaceSeparator;
    const auto isSpaceAscii = value == U'\t' || value == U' ';
    addRegexMask(result, isSpaceUnicode, isSpaceAscii, SpaceUnicode, SpaceAscii);

    const auto isSpaceUnicodeDotAll = isSpaceUnicode || isVerticalAscii ||
        unicodeCategory == UnicodeCategory::LineSeparator || unicodeCategory == UnicodeCategory::ParagraphSeparator;
    const auto isSpaceAsciiDotAll = isSpaceAscii || isVerticalAscii;
    addRegexMask(result, isSpaceUnicodeDotAll, isSpaceAsciiDotAll, SpaceUnicodeDotAll, SpaceAsciiDotAll);

    if (value != U'\n') {
        result |= static_cast<Mask>(Any);
    }
    result |= static_cast<Mask>(AnyDotAll);
    return result;
}

auto Category::maskFor(const Char character) noexcept -> Mask {
    constexpr auto cAsciiCharacterCount = std::size_t{128U};
    static const auto asciiMasks = []() {
        auto result = std::array<Mask, cAsciiCharacterCount>{};
        for (auto value = std::size_t{}; value < result.size(); ++value) {
            result[value] = computeMaskFor(Char{static_cast<char32_t>(value)});
        }
        return result;
    }();
    const auto value = character.toRawValue();
    if (value < asciiMasks.size()) {
        return asciiMasks[value];
    }
    return computeMaskFor(character);
}

auto Category::contains(const Char character) const noexcept -> bool {
    const auto characterMask = maskFor(character);
    return characterMask != 0U && (characterMask & mask()) == mask();
}

auto Category::characterSet() const -> CharSet {
    using UC = UnicodeCategory;
    using UG = UnicodeCategoryGroup;
    switch (_value) {
    case Other: {
        static const auto cCharacters = CharSet::from(UG::Other);
        return cCharacters;
    }
    case Letter: {
        static const auto cCharacters = CharSet::from(UG::Letter);
        return cCharacters;
    }
    case Mark: {
        static const auto cCharacters = CharSet::from(UG::Mark);
        return cCharacters;
    }
    case Number: {
        static const auto cCharacters = CharSet::from(UG::Number);
        return cCharacters;
    }
    case Punctuation: {
        static const auto cCharacters = CharSet::from(UG::Punctuation);
        return cCharacters;
    }
    case Symbol: {
        static const auto cCharacters = CharSet::from(UG::Symbol);
        return cCharacters;
    }
    case Separator: {
        static const auto cCharacters = CharSet::from(UG::Separator);
        return cCharacters;
    }
    case Control: {
        static const auto cCharacters = CharSet::from(UC::Control);
        return cCharacters;
    }
    case Format: {
        static const auto cCharacters = CharSet::from(UC::Format);
        return cCharacters;
    }
    case Unassigned: {
        static const auto cCharacters = CharSet::from(UC::Unassigned);
        return cCharacters;
    }
    case PrivateUse: {
        static const auto cCharacters = CharSet::from(UC::PrivateUse);
        return cCharacters;
    }
    case Surrogate: {
        static const auto cCharacters = CharSet::from(UC::Surrogate);
        return cCharacters;
    }
    case LowercaseLetter: {
        static const auto cCharacters = CharSet::from(UC::LowercaseLetter);
        return cCharacters;
    }
    case ModifierLetter: {
        static const auto cCharacters = CharSet::from(UC::ModifierLetter);
        return cCharacters;
    }
    case OtherLetter: {
        static const auto cCharacters = CharSet::from(UC::OtherLetter);
        return cCharacters;
    }
    case TitlecaseLetter: {
        static const auto cCharacters = CharSet::from(UC::TitlecaseLetter);
        return cCharacters;
    }
    case UppercaseLetter: {
        static const auto cCharacters = CharSet::from(UC::UppercaseLetter);
        return cCharacters;
    }
    case SpacingMark: {
        static const auto cCharacters = CharSet::from(UC::SpacingMark);
        return cCharacters;
    }
    case EnclosingMark: {
        static const auto cCharacters = CharSet::from(UC::EnclosingMark);
        return cCharacters;
    }
    case NonspacingMark: {
        static const auto cCharacters = CharSet::from(UC::NonspacingMark);
        return cCharacters;
    }
    case DigitUnicode:
    case DecimalNumber: {
        static const auto cCharacters = CharSet::from(UC::DecimalNumber);
        return cCharacters;
    }
    case LetterNumber: {
        static const auto cCharacters = CharSet::from(UC::LetterNumber);
        return cCharacters;
    }
    case OtherNumber: {
        static const auto cCharacters = CharSet::from(UC::OtherNumber);
        return cCharacters;
    }
    case ConnectorPunctuation: {
        static const auto cCharacters = CharSet::from(UC::ConnectorPunctuation);
        return cCharacters;
    }
    case DashPunctuation: {
        static const auto cCharacters = CharSet::from(UC::DashPunctuation);
        return cCharacters;
    }
    case ClosePunctuation: {
        static const auto cCharacters = CharSet::from(UC::ClosePunctuation);
        return cCharacters;
    }
    case FinalPunctuation: {
        static const auto cCharacters = CharSet::from(UC::FinalPunctuation);
        return cCharacters;
    }
    case InitialPunctuation: {
        static const auto cCharacters = CharSet::from(UC::InitialPunctuation);
        return cCharacters;
    }
    case OtherPunctuation: {
        static const auto cCharacters = CharSet::from(UC::OtherPunctuation);
        return cCharacters;
    }
    case OpenPunctuation: {
        static const auto cCharacters = CharSet::from(UC::OpenPunctuation);
        return cCharacters;
    }
    case CurrencySymbol: {
        static const auto cCharacters = CharSet::from(UC::CurrencySymbol);
        return cCharacters;
    }
    case ModifierSymbol: {
        static const auto cCharacters = CharSet::from(UC::ModifierSymbol);
        return cCharacters;
    }
    case MathSymbol: {
        static const auto cCharacters = CharSet::from(UC::MathSymbol);
        return cCharacters;
    }
    case OtherSymbol: {
        static const auto cCharacters = CharSet::from(UC::OtherSymbol);
        return cCharacters;
    }
    case LineSeparator: {
        static const auto cCharacters = CharSet::from(UC::LineSeparator);
        return cCharacters;
    }
    case ParagraphSeparator: {
        static const auto cCharacters = CharSet::from(UC::ParagraphSeparator);
        return cCharacters;
    }
    case SpaceSeparator: {
        static const auto cCharacters = CharSet::from(UC::SpaceSeparator);
        return cCharacters;
    }
    case DigitAscii:
        return CharSet::fromRange(U'0', U'9');
    case WordUnicode: {
        static const auto cCharacters = []() -> CharSet {
            auto result = CharSet::from(UG::Letter);
            result.add(CharSet::from(UC::DecimalNumber));
            result.add(U'_');
            return result;
        }();
        return cCharacters;
    }
    case WordAscii: {
        static const auto cCharacters = []() -> CharSet {
            auto result = CharSet::fromRange(U'0', U'9');
            result.add(CharSet::fromRange(U'A', U'Z'));
            result.add(CharSet::fromRange(U'a', U'z'));
            result.add(U'_');
            return result;
        }();
        return cCharacters;
    }
    case HorizontalSpaceUnicode:
    case SpaceUnicode: {
        static const auto cCharacters = []() -> CharSet {
            auto result = CharSet::from(UC::SpaceSeparator);
            result.add(U'\t');
            return result;
        }();
        return cCharacters;
    }
    case HorizontalSpaceAscii:
    case SpaceAscii: {
        static const auto cCharacters = CharSet{U'\t', U' ', Char{0x00A0U}};
        return cCharacters;
    }
    case SpaceUnicodeDotAll: {
        static const auto cCharacters = []() -> CharSet {
            auto result = CharSet::from(UC::SpaceSeparator);
            result.add(CharSet::from(UC::LineSeparator));
            result.add(CharSet::from(UC::ParagraphSeparator));
            result.add(CharSet{U'\t', U'\n', U'\v', U'\f', U'\r'});
            return result;
        }();
        return cCharacters;
    }
    case SpaceAsciiDotAll: {
        static const auto cCharacters = CharSet{U'\t', U' ', U'\n', U'\v', U'\f', U'\r', Char{0x00A0U}};
        return cCharacters;
    }
    case VerticalSpaceUnicode: {
        static const auto cCharacters = []() -> CharSet {
            auto result = CharSet{U'\n', U'\v', U'\f', U'\r', Char{0x0085U}};
            result.add(CharSet::from(UC::LineSeparator));
            result.add(CharSet::from(UC::ParagraphSeparator));
            return result;
        }();
        return cCharacters;
    }
    case VerticalSpaceAscii:
        return CharSet{U'\n', U'\v', U'\f', U'\r'};
    case Any: {
        auto result = CharSet::fromRange(Char{0U}, Char{0x10FFFFU});
        result.remove(U'\n');
        return result;
    }
    case AnyDotAll:
        return CharSet::fromRange(Char{0U}, Char{0x10FFFFU});
    case None:
        return {};
    }
    return {};
}

}
