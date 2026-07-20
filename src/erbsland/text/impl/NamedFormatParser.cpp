// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "NamedFormatParser.hpp"

#include "ThrowHelper.hpp"

namespace erbsland::text::impl {

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
    auto name = std::string{};
    while (reader.peek().isAsciiLetter()) {
        name.push_back(asciiLower(reader.read()));
    }
    if (name.empty() || !reader.advanceIf(U':')) {
        reader.restore(state);
        return {};
    }
    const auto domain = domainFromName(name);
    if (!domain.has_value()) {
        throwFormatError("Named format selector is not supported");
    }
    return domain;
}

auto NamedFormatParser::parse() -> FormatSpec {
    if (_reader.advanceIf(U'}')) {
        return _spec;
    }
    while (true) {
        if (currentChar() == U',' || currentChar() == U'}') {
            throwFormatError("Named format contains an empty option");
        }
        const auto optionName = readOptionName();
        const auto option = optionFromName(optionName);
        if (!option.has_value() || !isAllowed(option.value(), _domain)) {
            throwFormatError("Named format option is not supported for this selector");
        }
        markSeen(option.value());
        parseOption(option.value());
        if (_reader.advanceIf(U'}')) {
            break;
        }
        consumeExpected(U',', "Named format options must be separated by commas");
        if (currentChar() == U',' || currentChar() == U'}') {
            throwFormatError("Named format contains an empty option");
        }
    }
    validate();
    return _spec;
}

auto NamedFormatParser::asciiLower(const Char character) noexcept -> char {
    const auto value = character.toRawValue();
    return static_cast<char>(value >= U'A' && value <= U'Z' ? value + (U'a' - U'A') : value);
}

auto NamedFormatParser::isOptionNameChar(const Char character) noexcept -> bool {
    return character.isAsciiLetter() || character == U'-';
}

auto NamedFormatParser::isIdentifierChar(const Char character) noexcept -> bool {
    return character.isAsciiAlphanumeric() || character == U'-' || character == U'_';
}

auto NamedFormatParser::domainFromName(const std::string &name) -> std::optional<NamedFormatDomain> {
    if (name == "text") {
        return NamedFormatDomain::Text;
    }
    if (name == "number") {
        return NamedFormatDomain::Number;
    }
    if (name == "bool") {
        return NamedFormatDomain::Boolean;
    }
    if (name == "bytes") {
        return NamedFormatDomain::Bytes;
    }
    return {};
}

auto NamedFormatParser::optionFromName(const std::string &name) -> std::optional<Option> {
    if (name == "width" || name == "w") {
        return Option::Width;
    }
    if (name == "alignment" || name == "al") {
        return Option::Alignment;
    }
    if (name == "fill" || name == "fl") {
        return Option::Fill;
    }
    if (name == "maximum" || name == "max") {
        return Option::Maximum;
    }
    if (name == "escape" || name == "esc") {
        return Option::Escape;
    }
    if (name == "escape-amount" || name == "ea") {
        return Option::EscapeAmount;
    }
    if (name == "base" || name == "bs") {
        return Option::Base;
    }
    if (name == "notation" || name == "nt") {
        return Option::Notation;
    }
    if (name == "letter-case" || name == "lc") {
        return Option::LetterCase;
    }
    if (name == "sign" || name == "sg") {
        return Option::Sign;
    }
    if (name == "precision" || name == "pr") {
        return Option::Precision;
    }
    if (name == "alternate" || name == "alt") {
        return Option::Alternate;
    }
    if (name == "zero-fill" || name == "zf") {
        return Option::ZeroFill;
    }
    if (name == "style" || name == "sty") {
        return Option::Style;
    }
    if (name == "capitalization" || name == "cap") {
        return Option::Capitalization;
    }
    if (name == "separator" || name == "sep") {
        return Option::Separator;
    }
    if (name == "truncate" || name == "tr") {
        return Option::Truncate;
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

auto NamedFormatParser::currentChar() const noexcept -> Char {
    return _reader.peek();
}

auto NamedFormatParser::consumeSafeChar() -> Char {
    const auto character = _reader.read();
    if (character.isSignal()) {
        throwFormatError("Named format field is missing a closing brace");
    }
    if (!character.isSafeUnicode()) {
        throwFormatError("Named format contains an unsafe Unicode character");
    }
    return character;
}

void NamedFormatParser::consumeExpected(const Char expected, const std::string_view message) {
    if (!_reader.advanceIf(expected)) {
        throwFormatError(message);
    }
}

auto NamedFormatParser::readOptionName() -> std::string {
    auto result = std::string{};
    while (isOptionNameChar(currentChar())) {
        result.push_back(asciiLower(consumeSafeChar()));
    }
    if (result.empty()) {
        throwFormatError("Named format option name is missing");
    }
    return result;
}

auto NamedFormatParser::readIdentifier() -> std::string {
    consumeExpected(U'=', "Named format option is missing '='");
    auto result = std::string{};
    while (isIdentifierChar(currentChar())) {
        result.push_back(asciiLower(consumeSafeChar()));
    }
    if (result.empty()) {
        throwFormatError("Named format option value is missing");
    }
    return result;
}

auto NamedFormatParser::readNumber() -> unit::CpLength {
    _reader.advanceIf(U'=');
    if (!currentChar().isAsciiDigit()) {
        throwFormatError("Named numeric option value is missing");
    }
    auto result = uint32_t{};
    while (currentChar().isAsciiDigit()) {
        const auto digit = static_cast<uint32_t>(consumeSafeChar().toRawValue() - U'0');
        if (result > 102U || (result == 102U && digit > 4U)) {
            throwFormatError("Named numeric option value is too large");
        }
        result = result * 10U + digit;
    }
    return unit::CpLength{result};
}

auto NamedFormatParser::readFill() -> Char {
    consumeExpected(U'=', "Named fill option is missing '='");
    const auto result = consumeSafeChar();
    if (currentChar() != U',' && currentChar() != U'}') {
        throwFormatError("Named fill option requires exactly one character");
    }
    return result;
}

void NamedFormatParser::parseOption(const Option option) {
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
}

void NamedFormatParser::parseWidth() {
    layout().width = readNumber();
}

void NamedFormatParser::parseAlignment() {
    const auto value = readIdentifier();
    if (value == "left" || value == "l") {
        layout().alignment = bgeo::AlignmentFlag::Left;
    } else if (value == "right" || value == "r") {
        layout().alignment = bgeo::AlignmentFlag::Right;
    } else if (value == "center" || value == "c") {
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
    if (value == "none" || value == "n") {
        spec.escapeFormat = EscapeFormat::None;
    } else if (value == "html" || value == "h") {
        spec.escapeFormat = EscapeFormat::Html;
    } else if (value == "json" || value == "j") {
        spec.escapeFormat = EscapeFormat::Json;
    } else if (value == "cpp" || value == "cp") {
        spec.escapeFormat = EscapeFormat::Cpp;
    } else if (value == "xml" || value == "x") {
        spec.escapeFormat = EscapeFormat::Xml;
    } else if (value == "regex" || value == "rx") {
        spec.escapeFormat = EscapeFormat::RegEx;
    } else if (value == "display" || value == "d") {
        spec.escapeFormat = EscapeFormat::Display;
    } else if (value == "config" || value == "cf") {
        spec.escapeFormat = EscapeFormat::Config;
    } else if (value == "config_test" || value == "ct") {
        spec.escapeFormat = EscapeFormat::ConfigTest;
    } else {
        throwFormatError("Named escape value is not supported");
    }
}

void NamedFormatParser::parseEscapeAmount() {
    const auto value = readIdentifier();
    auto &spec = std::get<NamedTextFormatSpec>(_spec);
    spec.hasEscapeAmount = true;
    if (value == "nothing" || value == "n") {
        spec.escapeAmount = EscapeAmount::Nothing;
    } else if (value == "required" || value == "r") {
        spec.escapeAmount = EscapeAmount::Required;
    } else if (value == "balanced" || value == "b") {
        spec.escapeAmount = EscapeAmount::Balanced;
    } else if (value == "non-ascii" || value == "na") {
        spec.escapeAmount = EscapeAmount::NonAscii;
    } else if (value == "all" || value == "a") {
        spec.escapeAmount = EscapeAmount::Everything;
    } else {
        throwFormatError("Named escape amount is not supported");
    }
}

void NamedFormatParser::parseBase() {
    const auto value = readIdentifier();
    auto &spec = std::get<NamedNumberFormatSpec>(_spec);
    if (value == "decimal" || value == "d") {
        spec.base = IntegerBase::Decimal;
    } else if (value == "hexadecimal" || value == "x") {
        spec.base = IntegerBase::Hexadecimal;
    } else if (value == "binary" || value == "b") {
        spec.base = IntegerBase::Binary;
    } else if (value == "octal" || value == "o") {
        spec.base = IntegerBase::Octal;
    } else {
        throwFormatError("Named integer base is not supported");
    }
}

void NamedFormatParser::parseNotation() {
    const auto value = readIdentifier();
    auto &spec = std::get<NamedNumberFormatSpec>(_spec);
    if (value == "default" || value == "d") {
        spec.notation = FloatFormat::Style::Default;
    } else if (value == "fixed" || value == "f") {
        spec.notation = FloatFormat::Style::Fixed;
    } else if (value == "scientific" || value == "s") {
        spec.notation = FloatFormat::Style::Scientific;
    } else if (value == "general" || value == "g") {
        spec.notation = FloatFormat::Style::General;
    } else if (value == "hexadecimal" || value == "x") {
        spec.notation = FloatFormat::Style::Hexadecimal;
    } else {
        throwFormatError("Named floating-point notation is not supported");
    }
}

void NamedFormatParser::parseLetterCase() {
    const auto value = readIdentifier();
    auto &spec = std::get<NamedNumberFormatSpec>(_spec);
    if (value == "lowercase" || value == "l") {
        spec.letterCase = LetterCase::Lowercase;
    } else if (value == "uppercase" || value == "u") {
        spec.letterCase = LetterCase::Uppercase;
    } else {
        throwFormatError("Named letter case is not supported");
    }
}

void NamedFormatParser::parseSign() {
    const auto value = readIdentifier();
    auto &spec = std::get<NamedNumberFormatSpec>(_spec);
    if (value == "negative-only" || value == "n") {
        spec.signMode = IntegerSignMode::NegativeOnly;
    } else if (value == "always" || value == "a") {
        spec.signMode = IntegerSignMode::Always;
    } else if (value == "space" || value == "s") {
        spec.signMode = IntegerSignMode::Space;
    } else {
        throwFormatError("Named sign mode is not supported");
    }
}

void NamedFormatParser::parsePrecision() {
    std::get<NamedNumberFormatSpec>(_spec).precision = readNumber();
}

void NamedFormatParser::parseAlternate() {
    std::get<NamedNumberFormatSpec>(_spec).alternateForm = true;
}

void NamedFormatParser::parseZeroFill() {
    std::get<NamedNumberFormatSpec>(_spec).zeroFill = true;
}

void NamedFormatParser::parseStyle() {
    const auto value = readIdentifier();
    auto &format = std::get<NamedBooleanFormatSpec>(_spec).format;
    if (value == "true" || value == "t") {
        format.setStyle(BooleanFormat::Style::TrueFalse);
    } else if (value == "yes" || value == "y") {
        format.setStyle(BooleanFormat::Style::YesNo);
    } else if (value == "on" || value == "o") {
        format.setStyle(BooleanFormat::Style::OnOff);
    } else if (value == "enabled" || value == "e") {
        format.setStyle(BooleanFormat::Style::EnabledDisabled);
    } else {
        throwFormatError("Named boolean style is not supported");
    }
}

void NamedFormatParser::parseCapitalization() {
    const auto value = readIdentifier();
    auto &format = std::get<NamedBooleanFormatSpec>(_spec).format;
    if (value == "lowercase" || value == "l") {
        format.setCapitalization(Capitalization::Lowercase);
    } else if (value == "uppercase" || value == "u") {
        format.setCapitalization(Capitalization::Uppercase);
    } else if (value == "titlecase" || value == "t") {
        format.setCapitalization(Capitalization::Titlecase);
    } else {
        throwFormatError("Named capitalization is not supported");
    }
}

void NamedFormatParser::parseSeparator() {
    std::get<NamedBytesFormatSpec>(_spec).separator = true;
}

void NamedFormatParser::parseTruncate() {
    const auto value = readIdentifier();
    auto &spec = std::get<NamedBytesFormatSpec>(_spec);
    if (value == "end" || value == "e") {
        spec.truncateMode = TruncateMode::End;
    } else if (value == "middle" || value == "m") {
        spec.truncateMode = TruncateMode::Middle;
    } else if (value == "begin" || value == "b") {
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

void NamedFormatParser::markSeen(const Option option) {
    const auto mask = uint32_t{1U} << static_cast<uint32_t>(option);
    if ((_seenOptions & mask) != 0U) {
        throwFormatError("Named format option is specified more than once");
    }
    _seenOptions |= mask;
}

}
