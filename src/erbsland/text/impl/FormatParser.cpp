// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FormatParser.hpp"

#include "NamedFormatParser.hpp"

#include "../FormatError.hpp"
#include "../Literals.hpp"
#include "../StringEditor.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::text::impl {

using namespace text::literals;
using unit::ArgumentCount;
using unit::ArgumentIndex;
using unit::CpLength;

FormatParser::FormatParser(const U8String &pattern) :
    _reader{pattern}, _staticText{StringKind::U8}, _data{new FormatData{}} {
}

FormatParser::FormatParser(const U16String &pattern) :
    _reader{pattern}, _staticText{StringKind::U16}, _data{new FormatData{}} {
}

FormatParser::FormatParser(const U32String &pattern) :
    _reader{pattern}, _staticText{StringKind::U32}, _data{new FormatData{}} {
}

auto FormatParser::parse() -> FormatDataPtr {
    while (!_reader.isAtEnd()) {
        const auto next = consumeChar();
        if (next.isSignal()) {
            throw FormatError("Format pattern contains an invalid character"_el);
        }
        const auto character = next;
        if (character == U'{') {
            if (_reader.advanceIf(U'{')) {
                _staticText.append(U'{');
            } else {
                flushStaticText();
                parseField();
            }
        } else if (character == U'}') {
            if (_reader.advanceIf(U'}')) {
                _staticText.append(U'}');
            } else {
                throw FormatError("Format pattern contains an unmatched closing brace"_el);
            }
        } else {
            _staticText.append(character);
        }
    }
    return finish();
}

void FormatParser::flushStaticText() {
    if (!_staticText.isEmpty()) {
        _data->parts.emplace_back(FormatPart::fromStaticText(_staticText.takeAnyString()));
    }
}

void FormatParser::requireFieldLimit() const {
    if (_data->fieldCount >= cMaximumFields) {
        throw FormatError("Format pattern contains too many fields"_el);
    }
}

auto FormatParser::readIndex() -> std::optional<ArgumentIndex> {
    auto options = IntegerParseOptions::parserDefault();
    options.setFixedBase(IntegerBase::Decimal).setMaximumDigits(cMaximumArgumentIndexDigits);
    const auto result = _reader.parseInteger(options);
    if (result.status == ReadNumberStatus::NoDigits) {
        return {};
    }
    if (result.status != ReadNumberStatus::Success || result.value >= cMaximumFields.toSizeT()) {
        throw FormatError("Format argument index is too large"_el);
    }
    return ArgumentIndex::fromSizeT(static_cast<std::size_t>(result.value));
}

auto FormatParser::currentChar() const noexcept -> Char {
    return _reader.peek();
}

auto FormatParser::consumeChar() noexcept -> Char {
    return _reader.read();
}

auto FormatParser::consumeSpecificationChar() -> Char {
    const auto next = consumeChar();
    if (next.isEndOfData()) {
        throw FormatError("Format field is missing a closing brace"_el);
    }
    if (next.isSignal()) {
        throw FormatError("Format field contains an invalid character"_el);
    }
    const auto character = next;
    if (character == U'{') {
        throw FormatError("Format field contains an unexpected opening brace"_el);
    }
    if (!character.isAscii()) {
        throw FormatError("Format field specification must use ASCII characters"_el);
    }
    return character;
}

auto FormatParser::readLimitedDecimal(const StringLiteral &tooLargeMessage) -> CpLength {
    auto options = IntegerParseOptions::parserDefault();
    options.setFixedBase(IntegerBase::Decimal).setMaximumDigits(cMaximumDecimalDigits);
    const auto result = _reader.parseInteger(options);
    if (result.status == ReadNumberStatus::NoDigits) {
        return CpLength::zero();
    }
    if (result.status != ReadNumberStatus::Success || result.value > cMaximumFieldWidth.toRawValue()) {
        throw FormatError(tooLargeMessage);
    }
    return CpLength::fromSizeT(static_cast<std::size_t>(result.value));
}

auto FormatParser::parseSpecification() -> FormatSpec {
    if (currentChar() == U'}') {
        consumeChar();
        return LegacyFormatSpec{};
    }
    if (const auto domain = NamedFormatParser::tryReadDomain(_reader); domain.has_value()) {
        return NamedFormatParser{_reader, domain.value()}.parse();
    }
    return parseLegacySpecification();
}

auto FormatParser::parseLegacySpecification() -> LegacyFormatSpec {
    auto spec = LegacyFormatSpec{};
    parseAlignment(spec);
    parseSign(spec);
    if (currentChar() == U'#') {
        spec.alternateForm = true;
        consumeSpecificationChar();
    }
    if (currentChar() == U'0') {
        spec.zeroFill = true;
        consumeSpecificationChar();
    }
    parseWidth(spec);
    parsePrecision(spec);
    parsePresentation(spec);
    return spec;
}

void FormatParser::parseAlignment(LegacyFormatSpec &spec) {
    const auto first = currentChar();
    if (first == U'<' || first == U'>' || first == U'^') {
        consumeSpecificationChar();
        spec.alignment = first == U'<' ? geometry::AlignmentFlag::Left
            : first == U'>'            ? geometry::AlignmentFlag::Right
                                       : geometry::AlignmentFlag::HCenter;
        return;
    }
    if (first != U' ' && first != U'0') {
        return;
    }

    const auto state = _reader.save();
    consumeSpecificationChar();
    const auto second = currentChar();
    if (second == U'<' || second == U'>' || second == U'^') {
        consumeSpecificationChar();
        spec.fill = first;
        spec.alignment = second == U'<' ? geometry::AlignmentFlag::Left
            : second == U'>'            ? geometry::AlignmentFlag::Right
                                        : geometry::AlignmentFlag::HCenter;
        return;
    }
    _reader.restore(state);
}

void FormatParser::parseSign(LegacyFormatSpec &spec) {
    if (_reader.advanceIf(U'+')) {
        spec.signMode = IntegerSignMode::Always;
    } else if (_reader.advanceIf(U' ')) {
        spec.signMode = IntegerSignMode::Space;
    } else if (_reader.advanceIf(U'-')) {
        spec.signMode = IntegerSignMode::NegativeOnly;
    }
}

void FormatParser::parseWidth(LegacyFormatSpec &spec) {
    const auto width = readLimitedDecimal("Format field width is too large"_el);
    if (!width.isZero()) {
        spec.width = width;
    }
}

void FormatParser::parsePrecision(LegacyFormatSpec &spec) {
    if (!_reader.advanceIf(U'.')) {
        return;
    }
    auto options = IntegerParseOptions::parserDefault();
    options.setFixedBase(IntegerBase::Decimal).setMaximumDigits(cMaximumDecimalDigits);
    const auto result = _reader.parseInteger(options);
    if (result.status == ReadNumberStatus::NoDigits) {
        throw FormatError("Format field precision is missing digits"_el);
    }
    if (result.status != ReadNumberStatus::Success || result.value > cMaximumFieldWidth.toRawValue()) {
        throw FormatError("Format field precision is too large"_el);
    }
    spec.precision = CpLength::fromSizeT(static_cast<std::size_t>(result.value));
}

void FormatParser::parsePresentation(LegacyFormatSpec &spec) {
    const auto next = currentChar();
    if (next == U'}') {
        consumeSpecificationChar();
        return;
    }
    if (next == U'/') {
        parseEscapedPresentation(spec);
        return;
    }

    const auto type = consumeSpecificationChar();
    if (type == U'd') {
        spec.presentation = FormatPresentation::Decimal;
    } else if (type == U'x') {
        spec.presentation = FormatPresentation::Hex;
    } else if (type == U'X') {
        spec.presentation = FormatPresentation::Hex;
        spec.letterCase = LetterCase::Uppercase;
    } else if (type == U'b') {
        spec.presentation = FormatPresentation::Binary;
    } else if (type == U'B') {
        spec.presentation = FormatPresentation::Binary;
        spec.letterCase = LetterCase::Uppercase;
    } else if (type == U'o') {
        spec.presentation = FormatPresentation::Octal;
    } else if (type == U'O') {
        spec.presentation = FormatPresentation::Octal;
        spec.letterCase = LetterCase::Uppercase;
    } else if (type == U's') {
        spec.presentation = FormatPresentation::StringEditor;
    } else if (type == U'f') {
        spec.presentation = FormatPresentation::FloatFixed;
    } else if (type == U'F') {
        spec.presentation = FormatPresentation::FloatFixed;
        spec.letterCase = LetterCase::Uppercase;
    } else if (type == U'e') {
        spec.presentation = FormatPresentation::FloatScientific;
    } else if (type == U'E') {
        spec.presentation = FormatPresentation::FloatScientific;
        spec.letterCase = LetterCase::Uppercase;
    } else if (type == U'g') {
        spec.presentation = FormatPresentation::FloatGeneral;
    } else if (type == U'G') {
        spec.presentation = FormatPresentation::FloatGeneral;
        spec.letterCase = LetterCase::Uppercase;
    } else if (type == U'a') {
        spec.presentation = FormatPresentation::FloatHex;
    } else if (type == U'A') {
        spec.presentation = FormatPresentation::FloatHex;
        spec.letterCase = LetterCase::Uppercase;
    } else {
        throw FormatError("Format field type is not supported"_el);
    }
    if (currentChar() != U'}') {
        throw FormatError("Format field specification contains trailing characters"_el);
    }
    consumeSpecificationChar();
}

void FormatParser::parseEscapedPresentation(LegacyFormatSpec &spec) {
    consumeSpecificationChar(); // '/'
    auto formatName = StringEditor{};
    auto amount = EscapeAmount{EscapeAmount::Balanced};
    while (true) {
        const auto character = consumeSpecificationChar();
        if (character == U'}') {
            break;
        }
        const auto next = currentChar();
        if (next == U'}') {
            if (const auto suffixAmount = EscapeAmount::fromSuffix(character); suffixAmount.has_value()) {
                amount = suffixAmount.value();
                continue;
            }
        }
        formatName.append(character);
    }
    const auto format = EscapeFormat::fromString(formatName);
    if (!format.has_value()) {
        throw FormatError("Format escape modifier is not supported"_el);
    }
    spec.presentation = FormatPresentation::EscapedText;
    spec.escapeFormat = format.value();
    spec.escapeAmount = amount;
}

auto FormatParser::resolveArgumentIndex(const std::optional<ArgumentIndex> explicitIndex) -> ArgumentIndex {
    if (explicitIndex.has_value()) {
        if (_indexMode == IndexMode::Automatic) {
            throw FormatError("Format pattern mixes automatic and manual argument indexes"_el);
        }
        _indexMode = IndexMode::Manual;
        return explicitIndex.value();
    }
    if (_indexMode == IndexMode::Manual) {
        throw FormatError("Format pattern mixes automatic and manual argument indexes"_el);
    }
    _indexMode = IndexMode::Automatic;
    const auto result = _nextAutomaticIndex;
    ++_nextAutomaticIndex;
    return result;
}

void FormatParser::markArgumentIndex(const ArgumentIndex argumentIndex) {
    if (!argumentIndex.isWithin(cMaximumFields)) {
        throw FormatError("Format argument index is too large"_el);
    }
    const auto argumentIndexValue = argumentIndex.toSizeT();
    if (argumentIndexValue >= _data->usedArguments.size()) {
        _data->usedArguments.resize(argumentIndexValue + 1U, false);
    }
    _data->usedArguments[argumentIndexValue] = true;
    _data->argumentCount = std::max(_data->argumentCount, argumentIndex.distanceFromZero() + ArgumentCount::one());
}

void FormatParser::parseField() {
    requireFieldLimit();
    const auto argumentIndex = resolveArgumentIndex(readIndex());

    const auto separator = consumeChar();
    if (separator.isEndOfData()) {
        throw FormatError("Format field is missing a closing brace"_el);
    }
    if (separator.isSignal()) {
        throw FormatError("Format field contains an invalid character"_el);
    }
    auto spec = FormatSpec{LegacyFormatSpec{}};
    if (separator == U':') {
        spec = parseSpecification();
    } else if (separator != U'}') {
        throw FormatError("Format field contains unexpected characters after argument index"_el);
    }

    _data->parts.emplace_back(FormatPart::fromField(spec, argumentIndex));
    markArgumentIndex(argumentIndex);
    ++_data->fieldCount;
}

auto FormatParser::finish() -> FormatDataPtr {
    flushStaticText();
    for (
        auto argumentIndex = ArgumentIndex::zero(); argumentIndex.toSizeT() < _data->usedArguments.size();
        ++argumentIndex) {
        if (!_data->usedArguments[argumentIndex.toSizeT()]) {
            throw FormatError("Manual format argument indexes must be contiguous from zero"_el);
        }
    }
    return std::move(_data);
}

}
