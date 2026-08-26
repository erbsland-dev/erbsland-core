// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Category.hpp"

#include "../../../err/ParameterError.hpp"
#include "../../../text/Literals.hpp"

#include <algorithm>
#include <array>
#include <ranges>

namespace erbsland::re::impl {
using namespace text::literals;
using namespace text;

auto Category::fromString(const String &str) -> Category {
    if (const auto foundValue = nameToValueMap().get(str); foundValue.has_value()) {
        return *foundValue;
    }
    return {};
}

auto Category::fromUnprocessedString(const String &str) -> Category {
    const auto normalized = str.transformed([](const Char character) noexcept -> Char {
        return character == U'_' ? Char::noCodePoint() : character.toAsciiLowercase();
    });
    if (const auto foundValue = nameToValueMap().get(normalized); foundValue.has_value()) {
        return *foundValue;
    }
    throw err::ParameterError{"Invalid category name."_el, "str"_el};
}

auto Category::toLongString() const -> String {
    if (const auto foundName = valueToNameMap().find(_value); foundName != valueToNameMap().end()) {
        return foundName->second.unicodeLong;
    }
    return {};
}

auto Category::toShortString() const -> String {
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
            [&](const auto &x) -> bool {
                return std::ranges::any_of(
                    list, [&](const auto &other) -> bool { return x != other && other.includes(x); });
            })
            .begin(),
        list.end());
}

auto Category::nameToValueMap() noexcept -> const NameToValueMap & {
    static const NameToValueMap map{{
        {"other"_el, Other},
        {"letter"_el, Letter},
        {"mark"_el, Mark},
        {"number"_el, Number},
        {"punctuation"_el, Punctuation},
        {"symbol"_el, Symbol},
        {"separator"_el, Separator},
        {"control"_el, Control},
        {"format"_el, Format},
        {"unassigned"_el, Unassigned},
        {"privateuse"_el, PrivateUse},
        {"surrogate"_el, Surrogate},
        {"lowercaseletter"_el, LowercaseLetter},
        {"modifierletter"_el, ModifierLetter},
        {"otherletter"_el, OtherLetter},
        {"titlecaseletter"_el, TitlecaseLetter},
        {"uppercaseletter"_el, UppercaseLetter},
        {"spacingmark"_el, SpacingMark},
        {"enclosingmark"_el, EnclosingMark},
        {"nonspacingmark"_el, NonspacingMark},
        {"decimalnumber"_el, DecimalNumber},
        {"letternumber"_el, LetterNumber},
        {"othernumber"_el, OtherNumber},
        {"connectorpunctuation"_el, ConnectorPunctuation},
        {"dashpunctuation"_el, DashPunctuation},
        {"closepunctuation"_el, ClosePunctuation},
        {"finalpunctuation"_el, FinalPunctuation},
        {"initialpunctuation"_el, InitialPunctuation},
        {"otherpunctuation"_el, OtherPunctuation},
        {"openpunctuation"_el, OpenPunctuation},
        {"currencysymbol"_el, CurrencySymbol},
        {"modifiersymbol"_el, ModifierSymbol},
        {"mathsymbol"_el, MathSymbol},
        {"othersymbol"_el, OtherSymbol},
        {"lineseparator"_el, LineSeparator},
        {"paragraphseparator"_el, ParagraphSeparator},
        {"spaceseparator"_el, SpaceSeparator},
        {"c"_el, Other},
        {"l"_el, Letter},
        {"m"_el, Mark},
        {"n"_el, Number},
        {"p"_el, Punctuation},
        {"s"_el, Symbol},
        {"z"_el, Separator},
        {"cc"_el, Control},
        {"cf"_el, Format},
        {"cn"_el, Unassigned},
        {"co"_el, PrivateUse},
        {"cs"_el, Surrogate},
        {"ll"_el, LowercaseLetter},
        {"lm"_el, ModifierLetter},
        {"lo"_el, OtherLetter},
        {"lt"_el, TitlecaseLetter},
        {"lu"_el, UppercaseLetter},
        {"mc"_el, SpacingMark},
        {"me"_el, EnclosingMark},
        {"mn"_el, NonspacingMark},
        {"nd"_el, DecimalNumber},
        {"nl"_el, LetterNumber},
        {"no"_el, OtherNumber},
        {"pc"_el, ConnectorPunctuation},
        {"pd"_el, DashPunctuation},
        {"pe"_el, ClosePunctuation},
        {"pf"_el, FinalPunctuation},
        {"pi"_el, InitialPunctuation},
        {"po"_el, OtherPunctuation},
        {"ps"_el, OpenPunctuation},
        {"sc"_el, CurrencySymbol},
        {"sk"_el, ModifierSymbol},
        {"sm"_el, MathSymbol},
        {"so"_el, OtherSymbol},
        {"zl"_el, LineSeparator},
        {"zp"_el, ParagraphSeparator},
        {"zs"_el, SpaceSeparator},
        // regex classes
        {"DigitUnicode"_el, DigitUnicode},
        {"DigitAscii"_el, DigitAscii},
        {"WordUnicode"_el, WordUnicode},
        {"WordAscii"_el, WordAscii},
        {"SpaceUnicode"_el, SpaceUnicode},
        {"SpaceAscii"_el, SpaceAscii},
        {"SpaceUnicodeDotAll"_el, SpaceUnicodeDotAll},
        {"SpaceAsciiDotAll"_el, SpaceAsciiDotAll},
        {"HorizontalSpaceUnicode"_el, HorizontalSpaceUnicode},
        {"HorizontalSpaceAscii"_el, HorizontalSpaceAscii},
        {"VerticalSpaceUnicode"_el, VerticalSpaceUnicode},
        {"VerticalSpaceAscii"_el, VerticalSpaceAscii},
        {"Any"_el, Any},
        {"AnyDotAll"_el, AnyDotAll},
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
