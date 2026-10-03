// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ReplacementParser.hpp"

#include "../Limits.hpp"

#include "../../../text/AnyString.hpp"
#include "../../../text/StringFormat.hpp"
#include "../../RegExError.hpp"

#include <limits>
#include <utility>

namespace erbsland::re::impl {

using namespace text;
using namespace text::literals;
using namespace unit;

ReplacementParser::ReplacementParser(const String &expression, const CaptureGroupNames &groupNames) :
    _reader{expression}, _groupNames{groupNames} {
}

auto ReplacementParser::parse() -> Replacement {
    _replacement = Replacement{};
    parseContent();
    return std::move(*_replacement);
}

void ReplacementParser::validate() {
    _replacement.reset();
    parseContent();
}

void ReplacementParser::parseContent() {
    _reader.reset();
    while (!_reader.isAtEnd()) {
        _reader.startCapture();
        _reader.advanceUntil(CharSet{U'{', U'}'}, limits::maximumReplacementTextLength + CpCount::one());
        if (_reader.position().distanceFromZero() > limits::maximumReplacementTextLength) {
            throwError(
                StringFormat{"The maximum replacement-expression length is {} characters."_el}.build(
                    limits::maximumReplacementTextLength));
        }
        addCapturedText();
        if (_reader.isAtEnd()) {
            break;
        }
        const auto character = _reader.read(); // read either `{` or `}`
        _reader.startCapture();
        if (_reader.advanceIf(character)) {    // if a second `{` or `}` follows, capture this as static text.
            addCapturedText();
        } else if (character == U'}') {        // with no second `}`, a lone `}` is an error.
            throwError("Unexpected '}' in replacement expression"_el);
        } else {
            const auto index = parseExpression();
            if (_replacement) {
                _replacement->addCaptureGroup(index);
            }
        }
    }
}

void ReplacementParser::addCapturedText() {
    if (_replacement) {
        const auto text = _reader.takeCapture().toU8String();
        if (!text.isEmpty()) {
            _replacement->addStaticText(text);
        }
    }
}

auto ReplacementParser::parseExpression() -> CaptureGroupIndex {
    if (_reader.advanceIf(U'}')) {
        return 0U;
    }
    if (_reader.peek().isDigitValue(IntegerBase::Decimal)) {
        return parseGroupIndex();
    }
    if (_reader.peek().isAsciiWord()) {
        return parseGroupName();
    }
    if (_reader.isAtEnd()) {
        throwError("Unterminated replacement expression"_el);
    }
    throwError("Invalid character in replacement expression"_el);
}

auto ReplacementParser::parseGroupIndex() -> CaptureGroupIndex {
    const auto index = _reader.parseInteger(
        IntegerParseOptions{}
            .setFixedBase(IntegerBase::Decimal)
            .setMaximumDigits(limits::maximumReplacementTextLength));
    if (index.status != ReadNumberStatus::Success || index.value > std::numeric_limits<CaptureGroupIndex>::max()) {
        throwError("Capture group index is out of range"_el);
    }
    requireClosingBrace("Invalid character in replacement expression. Expected a group index"_el);
    if (ItemCount::fromInteger(index.value) > _groupNames.count()) {
        throwError("Group index not found in this pattern"_el);
    }
    return static_cast<CaptureGroupIndex>(index.value);
}

auto ReplacementParser::parseGroupName() -> CaptureGroupIndex {
    _reader.startCapture();
    _reader.advanceWhile(AsciiCategory::Word, limits::maximumReplacementTextLength);
    const auto groupName = _reader.takeCapture().toU8String();
    requireClosingBrace("Invalid character in replacement expression. Expected a group name"_el);
    auto index = CaptureGroupIndex{1};
    for (const auto &name : _groupNames) {
        if (name.compare(groupName, Char::compareCaseFolded) == std::strong_ordering::equal) {
            return index;
        }
        ++index;
    }
    throwError("Group name not found in this pattern"_el);
}

void ReplacementParser::requireClosingBrace(const String &description) {
    if (_reader.isAtEnd()) {
        throwError("Unterminated replacement expression"_el);
    }
    if (!_reader.advanceIf(U'}')) {
        throwError(description);
    }
}

void ReplacementParser::throwError(String description) const {
    throw RegExError{
        ErrorCategory::Format,
        "Failed to parse replacement expression"_el,
        std::move(description),
        CodeLocation{}.setPosition(_reader.position())};
}

}
