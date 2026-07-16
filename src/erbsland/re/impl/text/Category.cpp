// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Category.hpp"

#include "../../../err/ParameterError.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/String.hpp"

#include <algorithm>
#include <ranges>

namespace erbsland::re::impl {

using namespace text::literals;

auto Category::includes(const Category &other) const noexcept -> bool {
    return (mask() & other.mask()) == mask();
}

constexpr auto Category::unicodeMaskFor(const text::UnicodeCategory category) noexcept -> Mask {
    using UC = text::UnicodeCategory;
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

auto Category::maskFor(const text::Char character) noexcept -> Mask {
    if (!character.isValidUnicode()) {
        return 0U;
    }

    const auto unicodeCategory = character.category();
    const auto unicodeGroup = character.categoryGroup();
    const auto value = character.toRawValue();
    auto result = unicodeMaskFor(unicodeCategory);

    const auto isDecimalNumber = unicodeCategory == text::UnicodeCategory::DecimalNumber;
    addRegexMask(result, isDecimalNumber, character.isAsciiDigit(), DigitUnicode, DigitAscii);

    const auto isWordUnicode = unicodeGroup == text::UnicodeCategoryGroup::Letter || isDecimalNumber || value == U'_';
    const auto isWordAscii = character.isAsciiWord();
    addRegexMask(result, isWordUnicode, isWordAscii, WordUnicode, WordAscii);

    const auto isSpaceSeparator = unicodeCategory == text::UnicodeCategory::SpaceSeparator;
    const auto isHorizontalUnicode = value == U'\t' || isSpaceSeparator;
    // U+00A0 retains the imported engine's compatibility behavior. Its ASCII subcategory bit is shared with the
    // other regular-expression categories, so it also satisfies SpaceAscii below.
    const auto isHorizontalAscii = value == U'\t' || value == U' ' || value == 0x00A0U;
    addRegexMask(result, isHorizontalUnicode, isHorizontalAscii, HorizontalSpaceUnicode, HorizontalSpaceAscii);

    const auto isVerticalAscii = value == U'\n' || value == U'\v' || value == U'\f' || value == U'\r';
    const auto isVerticalUnicode = isVerticalAscii || value == 0x0085U ||
        unicodeCategory == text::UnicodeCategory::LineSeparator ||
        unicodeCategory == text::UnicodeCategory::ParagraphSeparator;
    addRegexMask(result, isVerticalUnicode, isVerticalAscii, VerticalSpaceUnicode, VerticalSpaceAscii);

    const auto isSpaceUnicode = value == U'\t' || isSpaceSeparator;
    const auto isSpaceAscii = value == U'\t' || value == U' ';
    addRegexMask(result, isSpaceUnicode, isSpaceAscii, SpaceUnicode, SpaceAscii);

    const auto isSpaceUnicodeDotAll = isSpaceUnicode || isVerticalAscii ||
        unicodeCategory == text::UnicodeCategory::LineSeparator ||
        unicodeCategory == text::UnicodeCategory::ParagraphSeparator;
    const auto isSpaceAsciiDotAll = isSpaceAscii || isVerticalAscii;
    addRegexMask(result, isSpaceUnicodeDotAll, isSpaceAsciiDotAll, SpaceUnicodeDotAll, SpaceAsciiDotAll);

    if (value != U'\n') {
        result |= static_cast<Mask>(Any);
    }
    result |= static_cast<Mask>(AnyDotAll);
    return result;
}

auto Category::contains(const text::Char character) const noexcept -> bool {
    const auto characterMask = maskFor(character);
    return characterMask != 0U && (characterMask & mask()) == mask();
}

auto Category::characterSet() const -> text::CharSet {
    using UC = text::UnicodeCategory;
    using UG = text::UnicodeCategoryGroup;
    switch (_value) {
    case Other:
        return text::CharSet::from(UG::Other);
    case Letter:
        return text::CharSet::from(UG::Letter);
    case Mark:
        return text::CharSet::from(UG::Mark);
    case Number:
        return text::CharSet::from(UG::Number);
    case Punctuation:
        return text::CharSet::from(UG::Punctuation);
    case Symbol:
        return text::CharSet::from(UG::Symbol);
    case Separator:
        return text::CharSet::from(UG::Separator);
    case Control:
        return text::CharSet::from(UC::Control);
    case Format:
        return text::CharSet::from(UC::Format);
    case Unassigned:
        return text::CharSet::from(UC::Unassigned);
    case PrivateUse:
        return text::CharSet::from(UC::PrivateUse);
    case Surrogate:
        return text::CharSet::from(UC::Surrogate);
    case LowercaseLetter:
        return text::CharSet::from(UC::LowercaseLetter);
    case ModifierLetter:
        return text::CharSet::from(UC::ModifierLetter);
    case OtherLetter:
        return text::CharSet::from(UC::OtherLetter);
    case TitlecaseLetter:
        return text::CharSet::from(UC::TitlecaseLetter);
    case UppercaseLetter:
        return text::CharSet::from(UC::UppercaseLetter);
    case SpacingMark:
        return text::CharSet::from(UC::SpacingMark);
    case EnclosingMark:
        return text::CharSet::from(UC::EnclosingMark);
    case NonspacingMark:
        return text::CharSet::from(UC::NonspacingMark);
    case DecimalNumber:
        return text::CharSet::from(UC::DecimalNumber);
    case LetterNumber:
        return text::CharSet::from(UC::LetterNumber);
    case OtherNumber:
        return text::CharSet::from(UC::OtherNumber);
    case ConnectorPunctuation:
        return text::CharSet::from(UC::ConnectorPunctuation);
    case DashPunctuation:
        return text::CharSet::from(UC::DashPunctuation);
    case ClosePunctuation:
        return text::CharSet::from(UC::ClosePunctuation);
    case FinalPunctuation:
        return text::CharSet::from(UC::FinalPunctuation);
    case InitialPunctuation:
        return text::CharSet::from(UC::InitialPunctuation);
    case OtherPunctuation:
        return text::CharSet::from(UC::OtherPunctuation);
    case OpenPunctuation:
        return text::CharSet::from(UC::OpenPunctuation);
    case CurrencySymbol:
        return text::CharSet::from(UC::CurrencySymbol);
    case ModifierSymbol:
        return text::CharSet::from(UC::ModifierSymbol);
    case MathSymbol:
        return text::CharSet::from(UC::MathSymbol);
    case OtherSymbol:
        return text::CharSet::from(UC::OtherSymbol);
    case LineSeparator:
        return text::CharSet::from(UC::LineSeparator);
    case ParagraphSeparator:
        return text::CharSet::from(UC::ParagraphSeparator);
    case SpaceSeparator:
        return text::CharSet::from(UC::SpaceSeparator);
    case DigitUnicode:
        return text::CharSet::from(UC::DecimalNumber);
    case DigitAscii:
        return text::CharSet::fromRange(U'0', U'9');
    case WordUnicode: {
        auto result = text::CharSet::from(UG::Letter);
        result.add(text::CharSet::from(UC::DecimalNumber));
        result.add(U'_');
        return result;
    }
    case WordAscii: {
        auto result = text::CharSet::fromRange(U'0', U'9');
        result.add(text::CharSet::fromRange(U'A', U'Z'));
        result.add(text::CharSet::fromRange(U'a', U'z'));
        result.add(U'_');
        return result;
    }
    case SpaceUnicode: {
        auto result = text::CharSet::from(UC::SpaceSeparator);
        result.add(U'\t');
        return result;
    }
    case SpaceAscii:
        return text::CharSet{U'\t', U' ', text::Char{0x00A0U}};
    case SpaceUnicodeDotAll: {
        auto result = text::CharSet::from(UC::SpaceSeparator);
        result.add(text::CharSet::from(UC::LineSeparator));
        result.add(text::CharSet::from(UC::ParagraphSeparator));
        result.add(text::CharSet{U'\t', U'\n', U'\v', U'\f', U'\r'});
        return result;
    }
    case SpaceAsciiDotAll:
        return text::CharSet{U'\t', U' ', U'\n', U'\v', U'\f', U'\r', text::Char{0x00A0U}};
    case HorizontalSpaceUnicode: {
        auto result = text::CharSet::from(UC::SpaceSeparator);
        result.add(U'\t');
        return result;
    }
    case HorizontalSpaceAscii:
        return text::CharSet{U'\t', U' ', text::Char{0x00A0U}};
    case VerticalSpaceUnicode: {
        auto result = text::CharSet{U'\n', U'\v', U'\f', U'\r', text::Char{0x0085U}};
        result.add(text::CharSet::from(UC::LineSeparator));
        result.add(text::CharSet::from(UC::ParagraphSeparator));
        return result;
    }
    case VerticalSpaceAscii:
        return text::CharSet{U'\n', U'\v', U'\f', U'\r'};
    case Any: {
        auto result = text::CharSet::fromRange(text::Char{0U}, text::Char{0x10FFFFU});
        result.remove(U'\n');
        return result;
    }
    case AnyDotAll:
        return text::CharSet::fromRange(text::Char{0U}, text::Char{0x10FFFFU});
    case None:
        return {};
    }
    return {};
}

auto Category::fromString(const text::StringView &str) -> Category {
    if (const auto foundValue = nameToValueMap().get(str); foundValue.has_value()) {
        return *foundValue;
    }
    return {};
}

auto Category::fromUnprocessedString(const text::StringView &str) -> Category {
    text::String normalized;
    static_cast<void>(str.forEach([&normalized](text::Char character) {
        character = character.caseFolded();
        if (character >= U'a' && character <= U'z') {
            normalized.append(character);
        } else if (character != U'_') {
            throw err::ParameterError{"Invalid category name."_el, "str"_el};
        }
        return util::LoopStatus::Continue;
    }));
    if (const auto foundValue = nameToValueMap().get(normalized); foundValue.has_value()) {
        return *foundValue;
    }
    throw err::ParameterError{"Invalid category name."_el, "str"_el};
}

auto Category::toLongString() const -> text::StringView {
    if (const auto foundName = valueToNameMap().find(_value); foundName != valueToNameMap().end()) {
        return foundName->second.unicodeLong;
    }
    return {};
}

auto Category::toShortString() const -> text::StringView {
    if (const auto foundName = valueToNameMap().find(_value); foundName != valueToNameMap().end()) {
        return foundName->second.unicodeShort;
    }
    return {};
}

void Category::normalizeList(std::vector<Category> &list) noexcept {
    std::ranges::sort(list);
    list.erase(std::ranges::unique(list).begin(), list.end());
    list.erase(
        std::ranges::remove_if(
            list,
            [&](const auto &x) {
                return std::ranges::any_of(list, [&](const auto &other) { return x != other && other.includes(x); });
            })
            .begin(),
        list.end());
}

auto Category::nameToValueMap() noexcept -> const NameToValueMap & {
    static const NameToValueMap map{{
        {"other"_els, Other},
        {"letter"_els, Letter},
        {"mark"_els, Mark},
        {"number"_els, Number},
        {"punctuation"_els, Punctuation},
        {"symbol"_els, Symbol},
        {"separator"_els, Separator},
        {"control"_els, Control},
        {"format"_els, Format},
        {"unassigned"_els, Unassigned},
        {"privateuse"_els, PrivateUse},
        {"surrogate"_els, Surrogate},
        {"lowercaseletter"_els, LowercaseLetter},
        {"modifierletter"_els, ModifierLetter},
        {"otherletter"_els, OtherLetter},
        {"titlecaseletter"_els, TitlecaseLetter},
        {"uppercaseletter"_els, UppercaseLetter},
        {"spacingmark"_els, SpacingMark},
        {"enclosingmark"_els, EnclosingMark},
        {"nonspacingmark"_els, NonspacingMark},
        {"decimalnumber"_els, DecimalNumber},
        {"letternumber"_els, LetterNumber},
        {"othernumber"_els, OtherNumber},
        {"connectorpunctuation"_els, ConnectorPunctuation},
        {"dashpunctuation"_els, DashPunctuation},
        {"closepunctuation"_els, ClosePunctuation},
        {"finalpunctuation"_els, FinalPunctuation},
        {"initialpunctuation"_els, InitialPunctuation},
        {"otherpunctuation"_els, OtherPunctuation},
        {"openpunctuation"_els, OpenPunctuation},
        {"currencysymbol"_els, CurrencySymbol},
        {"modifiersymbol"_els, ModifierSymbol},
        {"mathsymbol"_els, MathSymbol},
        {"othersymbol"_els, OtherSymbol},
        {"lineseparator"_els, LineSeparator},
        {"paragraphseparator"_els, ParagraphSeparator},
        {"spaceseparator"_els, SpaceSeparator},
        {"c"_els, Other},
        {"l"_els, Letter},
        {"m"_els, Mark},
        {"n"_els, Number},
        {"p"_els, Punctuation},
        {"s"_els, Symbol},
        {"z"_els, Separator},
        {"cc"_els, Control},
        {"cf"_els, Format},
        {"cn"_els, Unassigned},
        {"co"_els, PrivateUse},
        {"cs"_els, Surrogate},
        {"ll"_els, LowercaseLetter},
        {"lm"_els, ModifierLetter},
        {"lo"_els, OtherLetter},
        {"lt"_els, TitlecaseLetter},
        {"lu"_els, UppercaseLetter},
        {"mc"_els, SpacingMark},
        {"me"_els, EnclosingMark},
        {"mn"_els, NonspacingMark},
        {"nd"_els, DecimalNumber},
        {"nl"_els, LetterNumber},
        {"no"_els, OtherNumber},
        {"pc"_els, ConnectorPunctuation},
        {"pd"_els, DashPunctuation},
        {"pe"_els, ClosePunctuation},
        {"pf"_els, FinalPunctuation},
        {"pi"_els, InitialPunctuation},
        {"po"_els, OtherPunctuation},
        {"ps"_els, OpenPunctuation},
        {"sc"_els, CurrencySymbol},
        {"sk"_els, ModifierSymbol},
        {"sm"_els, MathSymbol},
        {"so"_els, OtherSymbol},
        {"zl"_els, LineSeparator},
        {"zp"_els, ParagraphSeparator},
        {"zs"_els, SpaceSeparator},
        // regex classes
        {"DigitUnicode"_els, DigitUnicode},
        {"DigitAscii"_els, DigitAscii},
        {"WordUnicode"_els, WordUnicode},
        {"WordAscii"_els, WordAscii},
        {"SpaceUnicode"_els, SpaceUnicode},
        {"SpaceAscii"_els, SpaceAscii},
        {"SpaceUnicodeDotAll"_els, SpaceUnicodeDotAll},
        {"SpaceAsciiDotAll"_els, SpaceAsciiDotAll},
        {"HorizontalSpaceUnicode"_els, HorizontalSpaceUnicode},
        {"HorizontalSpaceAscii"_els, HorizontalSpaceAscii},
        {"VerticalSpaceUnicode"_els, VerticalSpaceUnicode},
        {"VerticalSpaceAscii"_els, VerticalSpaceAscii},
        {"Any"_els, Any},
        {"AnyDotAll"_els, AnyDotAll},
    }};
    return map;
}

auto Category::valueToNameMap() noexcept -> const ValueToNameMap & {
    static const ValueToNameMap map = {
        {Other, Names{"C"_el, "Other"_el}},
        {Letter, Names{"L"_el, "Letter"_el}},
        {Mark, Names{"M"_el, "Mark"_el}},
        {Number, Names{"N"_el, "Number"_el}},
        {Punctuation, Names{"P"_el, "Punctuation"_el}},
        {Symbol, Names{"S"_el, "Symbol"_el}},
        {Separator, Names{"Z"_el, "Separator"_el}},
        {Control, Names{"Cc"_el, "Control"_el}},
        {Format, Names{"Cf"_el, "Format"_el}},
        {Unassigned, Names{"Cn"_el, "Unassigned"_el}},
        {PrivateUse, Names{"Co"_el, "PrivateUse"_el}},
        {Surrogate, Names{"Cs"_el, "Surrogate"_el}},
        {LowercaseLetter, Names{"Ll"_el, "LowercaseLetter"_el}},
        {ModifierLetter, Names{"Lm"_el, "ModifierLetter"_el}},
        {OtherLetter, Names{"Lo"_el, "OtherLetter"_el}},
        {TitlecaseLetter, Names{"Lt"_el, "TitlecaseLetter"_el}},
        {UppercaseLetter, Names{"Lu"_el, "UppercaseLetter"_el}},
        {SpacingMark, Names{"Mc"_el, "SpacingMark"_el}},
        {EnclosingMark, Names{"Me"_el, "EnclosingMark"_el}},
        {NonspacingMark, Names{"Mn"_el, "NonspacingMark"_el}},
        {DecimalNumber, Names{"Nd"_el, "DecimalNumber"_el}},
        {LetterNumber, Names{"Nl"_el, "LetterNumber"_el}},
        {OtherNumber, Names{"No"_el, "OtherNumber"_el}},
        {ConnectorPunctuation, Names{"Pc"_el, "ConnectorPunctuation"_el}},
        {DashPunctuation, Names{"Pd"_el, "DashPunctuation"_el}},
        {ClosePunctuation, Names{"Pe"_el, "ClosePunctuation"_el}},
        {FinalPunctuation, Names{"Pf"_el, "FinalPunctuation"_el}},
        {InitialPunctuation, Names{"Pi"_el, "InitialPunctuation"_el}},
        {OtherPunctuation, Names{"Po"_el, "OtherPunctuation"_el}},
        {OpenPunctuation, Names{"Ps"_el, "OpenPunctuation"_el}},
        {CurrencySymbol, Names{"Sc"_el, "CurrencySymbol"_el}},
        {ModifierSymbol, Names{"Sk"_el, "ModifierSymbol"_el}},
        {MathSymbol, Names{"Sm"_el, "MathSymbol"_el}},
        {OtherSymbol, Names{"So"_el, "OtherSymbol"_el}},
        {LineSeparator, Names{"Zl"_el, "LineSeparator"_el}},
        {ParagraphSeparator, Names{"Zp"_el, "ParagraphSeparator"_el}},
        {SpaceSeparator, Names{"Zs"_el, "SpaceSeparator"_el}},
        {DigitUnicode, Names{"DigitUnicode"_el, "DigitUnicode"_el}},
        {DigitAscii, Names{"DigitAscii"_el, "DigitAscii"_el}},
        {WordUnicode, Names{"WordUnicode"_el, "WordUnicode"_el}},
        {WordAscii, Names{"WordAscii"_el, "WordAscii"_el}},
        {SpaceUnicode, Names{"SpaceUnicode"_el, "SpaceUnicode"_el}},
        {SpaceAscii, Names{"SpaceAscii"_el, "SpaceAscii"_el}},
        {SpaceUnicodeDotAll, Names{"SpaceUnicodeDotAll"_el, "SpaceUnicodeDotAll"_el}},
        {SpaceAsciiDotAll, Names{"SpaceAsciiDotAll"_el, "SpaceAsciiDotAll"_el}},
        {HorizontalSpaceUnicode, Names{"HorizontalSpaceUnicode"_el, "HorizontalSpaceUnicode"_el}},
        {HorizontalSpaceAscii, Names{"HorizontalSpaceAscii"_el, "HorizontalSpaceAscii"_el}},
        {VerticalSpaceUnicode, Names{"VerticalSpaceUnicode"_el, "VerticalSpaceUnicode"_el}},
        {VerticalSpaceAscii, Names{"VerticalSpaceAscii"_el, "VerticalSpaceAscii"_el}},
        {Any, Names{"Any"_el, "Any"_el}},
        {AnyDotAll, Names{"AnyDotAll"_el, "AnyDotAll"_el}},
    };
    return map;
}

}
