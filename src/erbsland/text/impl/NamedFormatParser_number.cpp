// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "NamedFormatParser.hpp"

#include "ThrowHelper.hpp"

#include "../Literals.hpp"

#include <variant>

namespace erbsland::text::impl {

using namespace literals;

void NamedFormatParser::parseBase() {
    const auto value = readIdentifier();
    auto &spec = std::get<NamedNumberFormatSpec>(_spec);
    if (spec.base.has_value()) {
        throwFormatError("Named integer base must not be selected more than once");
    }
    if (value == "decimal"_el || value == "dec"_el || value == "d"_el) {
        spec.base = IntegerBase::Decimal;
    } else if (value == "hexadecimal"_el || value == "hex"_el || value == "x"_el) {
        spec.base = IntegerBase::Hexadecimal;
    } else if (value == "binary"_el || value == "bin"_el || value == "b"_el) {
        spec.base = IntegerBase::Binary;
    } else if (value == "octal"_el || value == "oct"_el || value == "o"_el) {
        spec.base = IntegerBase::Octal;
    } else {
        throwFormatError("Named integer base is not supported");
    }
}

void NamedFormatParser::parseBaseShortcut(const IntegerBase base) {
    requireKeyWithoutValue();
    auto &spec = std::get<NamedNumberFormatSpec>(_spec);
    if (spec.base.has_value()) {
        throwFormatError("Named integer base must not be selected more than once");
    }
    spec.base = base;
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
    } else if (value == "hexadecimal"_el || value == "hex"_el || value == "x"_el) {
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

void NamedFormatParser::parseBasePrefix() {
    requireKeyWithoutValue();
    std::get<NamedNumberFormatSpec>(_spec).basePrefix = true;
}

void NamedFormatParser::parseZeroFill() {
    requireKeyWithoutValue();
    std::get<NamedNumberFormatSpec>(_spec).zeroFill = true;
}

}
