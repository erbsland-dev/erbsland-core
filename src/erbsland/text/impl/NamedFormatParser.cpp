// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "NamedFormatParser.hpp"

#include "NamedKeyParser.hpp"

#include "../FormatError.hpp"
#include "../Literals.hpp"
#include "../StringEditor.hpp"

#include "../../err/ParseError.hpp"
#include "../../util/Set.hpp"

namespace erbsland::text::impl {

using namespace literals;

NamedFormatParser::NamedFormatParser(StringCharReader &reader, const NamedFormatDomain domain) :
    _reader{reader}, _domain{domain} {
    switch (_domain) {
    case NamedFormatDomain::Text:
        _spec = NamedTextFormatSpec{};
        break;
    case NamedFormatDomain::Number:
        _spec = NamedNumberFormatSpec{};
        break;
    case NamedFormatDomain::Boolean:
        _spec = NamedBooleanFormatSpec{};
        break;
    case NamedFormatDomain::Bytes:
        _spec = NamedBytesFormatSpec{};
        break;
    }
}

auto NamedFormatParser::tryReadDomain(StringCharReader &reader) -> std::optional<NamedFormatDomain> {
    const auto state = reader.save();
    auto name = StringEditor{};
    while (reader.peek().isAsciiLetter()) {
        name.append(reader.read().toAsciiLowercase());
    }
    if (name.isEmpty() || !reader.advanceIf(U':')) {
        reader.restore(state);
        return {};
    }
    const auto domain = domainFromName(String{name});
    if (!domain.has_value()) {
        throwFormatError("Named format selector is not supported");
    }
    return domain;
}

auto NamedFormatParser::parse() -> FormatSpec {
    try {
        auto parser = NamedKeyParser{_reader, namedKeyFormat()};
        auto allowedKeys = util::Set<int>{};
        for (auto raw = uint8_t{}; raw <= static_cast<uint8_t>(Option::Truncate); ++raw) {
            const auto option = static_cast<Option>(raw);
            if (isAllowed(option, _domain)) {
                allowedKeys.insert(static_cast<int>(option));
            }
        }
        parser.setAllowedKeys(std::move(allowedKeys));
        while (true) {
            const auto entry = parser.readEntry();
            if (entry.isEnd()) {
                break;
            }
            parseOption(static_cast<Option>(entry.keyIndex()), entry);
        }
    } catch (const err::ParseError &error) {
        throw FormatError(error.reason());
    }
    validate();
    return _spec;
}

auto NamedFormatParser::namedKeyFormat() -> const NamedKeyFormat & {
    static const auto keys = NamedKeyFormat::Keys{{
        {"width"_el, static_cast<int>(Option::Width)},
        {"w"_el, static_cast<int>(Option::Width)},
        {"alignment"_el, static_cast<int>(Option::Alignment)},
        {"al"_el, static_cast<int>(Option::Alignment)},
        {"fill"_el, static_cast<int>(Option::Fill)},
        {"fl"_el, static_cast<int>(Option::Fill)},
        {"maximum"_el, static_cast<int>(Option::Maximum)},
        {"max"_el, static_cast<int>(Option::Maximum)},
        {"escape"_el, static_cast<int>(Option::Escape)},
        {"esc"_el, static_cast<int>(Option::Escape)},
        {"escape-amount"_el, static_cast<int>(Option::EscapeAmount)},
        {"ea"_el, static_cast<int>(Option::EscapeAmount)},
        {"base"_el, static_cast<int>(Option::Base)},
        {"bs"_el, static_cast<int>(Option::Base)},
        {"notation"_el, static_cast<int>(Option::Notation)},
        {"nt"_el, static_cast<int>(Option::Notation)},
        {"letter-case"_el, static_cast<int>(Option::LetterCase)},
        {"lc"_el, static_cast<int>(Option::LetterCase)},
        {"sign"_el, static_cast<int>(Option::Sign)},
        {"sg"_el, static_cast<int>(Option::Sign)},
        {"precision"_el, static_cast<int>(Option::Precision)},
        {"pr"_el, static_cast<int>(Option::Precision)},
        {"alternate"_el, static_cast<int>(Option::Alternate)},
        {"alt"_el, static_cast<int>(Option::Alternate)},
        {"zero-fill"_el, static_cast<int>(Option::ZeroFill)},
        {"zf"_el, static_cast<int>(Option::ZeroFill)},
        {"style"_el, static_cast<int>(Option::Style)},
        {"sty"_el, static_cast<int>(Option::Style)},
        {"capitalization"_el, static_cast<int>(Option::Capitalization)},
        {"cap"_el, static_cast<int>(Option::Capitalization)},
        {"separator"_el, static_cast<int>(Option::Separator)},
        {"sep"_el, static_cast<int>(Option::Separator)},
        {"truncate"_el, static_cast<int>(Option::Truncate)},
        {"tr"_el, static_cast<int>(Option::Truncate)},
    }};
    static const auto format = NamedKeyFormat{}
                                   .setKeys(keys)
                                   .setValueListAllowed(false)
                                   .setStopCharacter(U'}')
                                   .setValueWithoutKeySeparatorChars(CharSet::from(AsciiCategory::Digit));
    return format;
}

auto NamedFormatParser::domainFromName(const String &name) -> std::optional<NamedFormatDomain> {
    if (name == "text"_el) {
        return NamedFormatDomain::Text;
    }
    if (name == "number"_el) {
        return NamedFormatDomain::Number;
    }
    if (name == "bool"_el) {
        return NamedFormatDomain::Boolean;
    }
    if (name == "bytes"_el) {
        return NamedFormatDomain::Bytes;
    }
    return {};
}

auto NamedFormatParser::isAllowed(const Option option, const NamedFormatDomain domain) noexcept -> bool {
    switch (domain) {
    case NamedFormatDomain::Text:
        return option == Option::Width || option == Option::Alignment || option == Option::Fill ||
            option == Option::Maximum || option == Option::Escape || option == Option::EscapeAmount;
    case NamedFormatDomain::Number:
        return option == Option::Width || option == Option::Alignment || option == Option::Fill ||
            option == Option::Base || option == Option::Notation || option == Option::LetterCase ||
            option == Option::Sign || option == Option::Precision || option == Option::Alternate ||
            option == Option::ZeroFill;
    case NamedFormatDomain::Boolean:
        return option == Option::Width || option == Option::Alignment || option == Option::Fill ||
            option == Option::Style || option == Option::Capitalization;
    case NamedFormatDomain::Bytes:
        return option == Option::Separator || option == Option::Maximum || option == Option::Truncate;
    }
    return false;
}

auto NamedFormatParser::readIdentifier() const -> String {
    if (!_entry->isKeyWithValue()) {
        throwFormatError("Named format option value is missing");
    }
    auto reader = StringCharReader{_entry->value()};
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        if (!character.isAsciiAlphanumeric() && character != U'-' && character != U'_') {
            throwFormatError("Named format option value is not an identifier");
        }
    }
    return _entry->value().transformed(Char::toAsciiLowercase);
}

auto NamedFormatParser::readNumber() -> unit::CpLength {
    if (!_entry->isKeyWithValue()) {
        throwFormatError("Named numeric option value is missing");
    }
    auto reader = StringCharReader{_entry->value()};
    auto result = uint32_t{};
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        if (!character.isAsciiDigit()) {
            throwFormatError("Named numeric option value must contain decimal digits");
        }
        const auto digit = static_cast<uint32_t>(character.toRawValue() - U'0');
        if (result > 102U || (result == 102U && digit > 4U)) {
            throwFormatError("Named numeric option value is too large");
        }
        result = result * 10U + digit;
    }
    return unit::CpLength{result};
}

auto NamedFormatParser::readFill() const -> Char {
    if (!_entry->isKeyWithValue()) {
        throwFormatError("Named fill option is missing '='");
    }
    if (_entry->value().characterLength() != unit::CpLength::one()) {
        throwFormatError("Named fill option requires exactly one character");
    }
    return _entry->value().charAt(StringSide::Front);
}

void NamedFormatParser::parseOption(const Option option, const NamedKeyEntry &entry) {
    _entry = &entry;
    switch (option) {
    case Option::Width:
        parseWidth();
        break;
    case Option::Alignment:
        parseAlignment();
        break;
    case Option::Fill:
        parseFill();
        break;
    case Option::Maximum:
        parseMaximum();
        break;
    case Option::Escape:
        parseEscape();
        break;
    case Option::EscapeAmount:
        parseEscapeAmount();
        break;
    case Option::Base:
        parseBase();
        break;
    case Option::Notation:
        parseNotation();
        break;
    case Option::LetterCase:
        parseLetterCase();
        break;
    case Option::Sign:
        parseSign();
        break;
    case Option::Precision:
        parsePrecision();
        break;
    case Option::Alternate:
        parseAlternate();
        break;
    case Option::ZeroFill:
        parseZeroFill();
        break;
    case Option::Style:
        parseStyle();
        break;
    case Option::Capitalization:
        parseCapitalization();
        break;
    case Option::Separator:
        parseSeparator();
        break;
    case Option::Truncate:
        parseTruncate();
        break;
    }
    _entry = nullptr;
}

void NamedFormatParser::requireKeyWithoutValue() const {
    if (!_entry->isKey()) {
        throwFormatError("Named format flag must not have a value");
    }
}

void NamedFormatParser::parseWidth() {
    layout().width = readNumber();
}

void NamedFormatParser::parseAlignment() {
    const auto value = readIdentifier();
    if (value == "left"_el || value == "l"_el) {
        layout().alignment = bgeo::AlignmentFlag::Left;
    } else if (value == "right"_el || value == "r"_el) {
        layout().alignment = bgeo::AlignmentFlag::Right;
    } else if (value == "center"_el || value == "c"_el) {
        layout().alignment = bgeo::AlignmentFlag::HCenter;
    } else {
        throwFormatError("Named alignment value is not supported");
    }
}

void NamedFormatParser::parseFill() {
    layout().fill = readFill();
}

void NamedFormatParser::parseMaximum() {
    const auto value = readNumber();
    if (_domain == NamedFormatDomain::Text) {
        std::get<NamedTextFormatSpec>(_spec).maximum = value;
    } else {
        std::get<NamedBytesFormatSpec>(_spec).maximum = unit::ByteLength{value.toRawValue()};
    }
}

void NamedFormatParser::parseEscape() {
    const auto value = readIdentifier();
    auto &spec = std::get<NamedTextFormatSpec>(_spec);
    if (value == "none"_el || value == "n"_el) {
        spec.escapeFormat = EscapeFormat::None;
    } else if (value == "html"_el || value == "h"_el) {
        spec.escapeFormat = EscapeFormat::Html;
    } else if (value == "json"_el || value == "j"_el) {
        spec.escapeFormat = EscapeFormat::Json;
    } else if (value == "cpp"_el || value == "cp"_el) {
        spec.escapeFormat = EscapeFormat::Cpp;
    } else if (value == "xml"_el || value == "x"_el) {
        spec.escapeFormat = EscapeFormat::Xml;
    } else if (value == "regex"_el || value == "rx"_el) {
        spec.escapeFormat = EscapeFormat::RegEx;
    } else if (value == "display"_el || value == "d"_el) {
        spec.escapeFormat = EscapeFormat::Display;
    } else if (value == "config"_el || value == "cf"_el) {
        spec.escapeFormat = EscapeFormat::Config;
    } else if (value == "config_test"_el || value == "ct"_el) {
        spec.escapeFormat = EscapeFormat::ConfigTest;
    } else {
        throwFormatError("Named escape value is not supported");
    }
}

void NamedFormatParser::parseEscapeAmount() {
    const auto value = readIdentifier();
    auto &spec = std::get<NamedTextFormatSpec>(_spec);
    spec.hasEscapeAmount = true;
    if (value == "nothing"_el || value == "n"_el) {
        spec.escapeAmount = EscapeAmount::Nothing;
    } else if (value == "required"_el || value == "r"_el) {
        spec.escapeAmount = EscapeAmount::Required;
    } else if (value == "balanced"_el || value == "b"_el) {
        spec.escapeAmount = EscapeAmount::Balanced;
    } else if (value == "non-ascii"_el || value == "na"_el) {
        spec.escapeAmount = EscapeAmount::NonAscii;
    } else if (value == "all"_el || value == "a"_el) {
        spec.escapeAmount = EscapeAmount::Everything;
    } else {
        throwFormatError("Named escape amount is not supported");
    }
}

void NamedFormatParser::parseBase() {
    const auto value = readIdentifier();
    auto &spec = std::get<NamedNumberFormatSpec>(_spec);
    if (value == "decimal"_el || value == "d"_el) {
        spec.base = IntegerBase::Decimal;
    } else if (value == "hexadecimal"_el || value == "x"_el) {
        spec.base = IntegerBase::Hexadecimal;
    } else if (value == "binary"_el || value == "b"_el) {
        spec.base = IntegerBase::Binary;
    } else if (value == "octal"_el || value == "o"_el) {
        spec.base = IntegerBase::Octal;
    } else {
        throwFormatError("Named integer base is not supported");
    }
}

void NamedFormatParser::parseNotation() {
    const auto value = readIdentifier();
    auto &spec = std::get<NamedNumberFormatSpec>(_spec);
    if (value == "default"_el || value == "d"_el) {
        spec.notation = FloatFormat::Style::Default;
    } else if (value == "fixed"_el || value == "f"_el) {
        spec.notation = FloatFormat::Style::Fixed;
    } else if (value == "scientific"_el || value == "s"_el) {
        spec.notation = FloatFormat::Style::Scientific;
    } else if (value == "general"_el || value == "g"_el) {
        spec.notation = FloatFormat::Style::General;
    } else if (value == "hexadecimal"_el || value == "x"_el) {
        spec.notation = FloatFormat::Style::Hexadecimal;
    } else {
        throwFormatError("Named floating-point notation is not supported");
    }
}

void NamedFormatParser::parseLetterCase() {
    const auto value = readIdentifier();
    auto &spec = std::get<NamedNumberFormatSpec>(_spec);
    if (value == "lowercase"_el || value == "l"_el) {
        spec.letterCase = LetterCase::Lowercase;
    } else if (value == "uppercase"_el || value == "u"_el) {
        spec.letterCase = LetterCase::Uppercase;
    } else {
        throwFormatError("Named letter case is not supported");
    }
}

void NamedFormatParser::parseSign() {
    const auto value = readIdentifier();
    auto &spec = std::get<NamedNumberFormatSpec>(_spec);
    if (value == "negative-only"_el || value == "n"_el) {
        spec.signMode = IntegerSignMode::NegativeOnly;
    } else if (value == "always"_el || value == "a"_el) {
        spec.signMode = IntegerSignMode::Always;
    } else if (value == "space"_el || value == "s"_el) {
        spec.signMode = IntegerSignMode::Space;
    } else {
        throwFormatError("Named sign mode is not supported");
    }
}

void NamedFormatParser::parsePrecision() {
    std::get<NamedNumberFormatSpec>(_spec).precision = readNumber();
}

void NamedFormatParser::parseAlternate() {
    requireKeyWithoutValue();
    std::get<NamedNumberFormatSpec>(_spec).alternateForm = true;
}

void NamedFormatParser::parseZeroFill() {
    requireKeyWithoutValue();
    std::get<NamedNumberFormatSpec>(_spec).zeroFill = true;
}

void NamedFormatParser::parseStyle() {
    const auto value = readIdentifier();
    auto &format = std::get<NamedBooleanFormatSpec>(_spec).format;
    if (value == "true"_el || value == "t"_el) {
        format.setStyle(BooleanFormat::Style::TrueFalse);
    } else if (value == "yes"_el || value == "y"_el) {
        format.setStyle(BooleanFormat::Style::YesNo);
    } else if (value == "on"_el || value == "o"_el) {
        format.setStyle(BooleanFormat::Style::OnOff);
    } else if (value == "enabled"_el || value == "e"_el) {
        format.setStyle(BooleanFormat::Style::EnabledDisabled);
    } else {
        throwFormatError("Named boolean style is not supported");
    }
}

void NamedFormatParser::parseCapitalization() {
    const auto value = readIdentifier();
    auto &format = std::get<NamedBooleanFormatSpec>(_spec).format;
    if (value == "lowercase"_el || value == "l"_el) {
        format.setCapitalization(Capitalization::Lowercase);
    } else if (value == "uppercase"_el || value == "u"_el) {
        format.setCapitalization(Capitalization::Uppercase);
    } else if (value == "titlecase"_el || value == "t"_el) {
        format.setCapitalization(Capitalization::Titlecase);
    } else {
        throwFormatError("Named capitalization is not supported");
    }
}

void NamedFormatParser::parseSeparator() {
    requireKeyWithoutValue();
    std::get<NamedBytesFormatSpec>(_spec).separator = true;
}

void NamedFormatParser::parseTruncate() {
    const auto value = readIdentifier();
    auto &spec = std::get<NamedBytesFormatSpec>(_spec);
    if (value == "end"_el || value == "e"_el) {
        spec.truncateMode = TruncateMode::End;
    } else if (value == "middle"_el || value == "m"_el) {
        spec.truncateMode = TruncateMode::Middle;
    } else if (value == "begin"_el || value == "b"_el) {
        spec.truncateMode = TruncateMode::Begin;
    } else {
        throwFormatError("Named truncation mode is not supported");
    }
}

auto NamedFormatParser::layout() -> NamedLayoutSpec & {
    switch (_domain) {
    case NamedFormatDomain::Text:
        return std::get<NamedTextFormatSpec>(_spec).layout;
    case NamedFormatDomain::Number:
        return std::get<NamedNumberFormatSpec>(_spec).layout;
    case NamedFormatDomain::Boolean:
        return std::get<NamedBooleanFormatSpec>(_spec).layout;
    case NamedFormatDomain::Bytes:
        break;
    }
    throwFormatError("Named bytes format does not support field layout");
}

void NamedFormatParser::validate() const {
    if (_domain == NamedFormatDomain::Text) {
        const auto &spec = std::get<NamedTextFormatSpec>(_spec);
        if (spec.hasEscapeAmount &&
            (!spec.escapeFormat.has_value() || spec.escapeFormat.value() == EscapeFormat::None)) {
            throwFormatError("Named escape amount requires a non-empty escape format");
        }
    } else if (_domain == NamedFormatDomain::Number) {
        const auto &spec = std::get<NamedNumberFormatSpec>(_spec);
        if (spec.base.has_value() && spec.notation.has_value()) {
            throwFormatError("Named number format cannot combine base and notation");
        }
        if (spec.notation.has_value() && spec.alternateForm) {
            throwFormatError("Named floating-point notation does not support alternate form");
        }
        if (spec.zeroFill && spec.layout.fill != U' ') {
            throwFormatError("Named number format cannot combine zero-fill with a custom fill");
        }
    }
}

}
