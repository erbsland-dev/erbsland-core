// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ReplacementParser.hpp"

#include "../Limits.hpp"

#include "../../../text/StringFormat.hpp"
#include "../../RegExError.hpp"

#include <limits>

namespace erbsland::re::impl {

using namespace text::literals;

ReplacementParser::ReplacementParser(const text::String &expression, const CaptureGroupNames &groupNames) :
    _reader{expression}, _nameMap{createCaptureGroupNameToIndexMap(groupNames)} {
}

auto ReplacementParser::parse() -> Replacement {
    if (_reader.isAtEnd()) {
        return {};
    }
    readNext();
    while (!_currentChar.isEndOfData()) {
        switch (_currentChar.toRawValue()) {
        case U'{':
            readNext();
            requireMoreContent();
            if (_currentChar == U'{') {
                _staticText.append(_currentChar);
                readNext();
            } else {
                if (!_staticText.isEmpty()) {
                    _replacement.addStaticText(_staticText);
                    _staticText.clear();
                }
                parseExpression();
            }
            break;
        case U'}':
            readNext();
            if (_currentChar == U'}') {
                _staticText.append(_currentChar);
                readNext();
            } else {
                throwError("Unexpected '}' in replacement expression"_el);
            }
            break;
        default:
            _staticText.append(_currentChar);
            readNext();
            break;
        }
    }
    if (!_staticText.isEmpty()) {
        _replacement.addStaticText(_staticText);
    }
    return _replacement;
}

void ReplacementParser::readNext() {
    if (_reader.position().distanceFromZero() > limits::maximumReplacementTextLength) {
        throwError(
            text::StringFormat{"The maximum replacement-expression length is {} characters."_el}.build(
                limits::maximumReplacementTextLength));
    }
    _currentChar = _reader.read();
}

void ReplacementParser::parseExpression() {
    if (_currentChar.isDigitValue(text::IntegerBase::Decimal)) {
        parseGroupIndex();
    } else if (_currentChar.isAsciiWord()) {
        parseGroupName();
    } else if (_currentChar == U'}') {
        _replacement.addCaptureGroup(0U);
        readNext();
    } else {
        throwError("Invalid character in replacement expression"_el);
    }
}

void ReplacementParser::parseGroupIndex() {
    auto groupIndex = std::uint32_t{};
    while (_currentChar.isDigitValue(text::IntegerBase::Decimal)) {
        groupIndex = (groupIndex * 10U) + _currentChar.digitValue().value();
        if (groupIndex > std::numeric_limits<CaptureGroupIndex>::max()) {
            throwError("Capture group index is out of range"_el);
        }
        readNext();
    }
    requireMoreContent();
    if (_currentChar != U'}') {
        throwError("Invalid character in replacement expression. Expected a group index"_el);
    }
    readNext();
    _replacement.addCaptureGroup(static_cast<CaptureGroupIndex>(groupIndex));
}

void ReplacementParser::parseGroupName() {
    text::StringEditor groupName;
    while (_currentChar.isAsciiWord()) {
        groupName.append(_currentChar);
        readNext();
    }
    requireMoreContent();
    if (_currentChar != U'}') {
        throwError("Invalid character in replacement expression. Expected a group name"_el);
    }
    readNext();
    const auto groupIndex = _nameMap.get(groupName);
    if (!groupIndex.has_value()) {
        throwError("Group name not found in this pattern"_el);
    }
    _replacement.addCaptureGroup(groupIndex.value());
}

void ReplacementParser::requireMoreContent() {
    if (_currentChar.isEndOfData()) {
        throwError("Unterminated replacement expression"_el);
    }
}

void ReplacementParser::throwError(const text::String &description) const {
    auto position = _reader.position();
    if (!_currentChar.isSignal()) {
        position -= unit::CpLength::one();
    }
    throw RegExError{
        ErrorCategory::Format,
        "Failed to parse replacement expression"_el,
        description,
        unit::CodeLocation{.position = position}};
}

}
