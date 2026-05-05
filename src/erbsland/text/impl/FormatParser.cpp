// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FormatParser.hpp"

#include "../Literals.hpp"
#include "../String.hpp"

#include "../../err/ThrowHelper.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::text::impl {

using namespace erbsland::text::literals;

FormatParser::FormatParser(const U8StringView &pattern) :
    _reader{pattern}, _staticText{StringKind::U8}, _data{new FormatData{}} {
}

FormatParser::FormatParser(const U16StringView &pattern) :
    _reader{pattern}, _staticText{StringKind::U16}, _data{new FormatData{}} {
}

FormatParser::FormatParser(const U32StringView &pattern) :
    _reader{pattern}, _staticText{StringKind::U32}, _data{new FormatData{}} {
}

auto FormatParser::parse() -> FormatDataPtr {
    while (!_reader.isAtEnd()) {
        const auto next = consumeChar();
        if (next.isSignal()) {
            err::throwFormatError("Format pattern contains an invalid character"_el);
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
                err::throwFormatError("Format pattern contains an unmatched closing brace"_el);
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
        err::throwFormatError("Format pattern contains too many fields"_el);
    }
}

auto FormatParser::readIndex() -> std::optional<unit::ArgumentIndex> {
    auto options = IntegerParseOptions::parserDefault();
    options.setFixedBase(IntegerBase::Decimal).setMaximumDigits(cMaximumArgumentIndexDigits);
    const auto result = _reader.parseInteger(options);
    if (result.status == ReadNumberStatus::NoDigits) {
        return {};
    }
    if (result.status != ReadNumberStatus::Success || result.value >= cMaximumFields.toSizeT()) {
        err::throwFormatError("Format argument index is too large"_el);
    }
    return unit::ArgumentIndex::fromSizeT(static_cast<std::size_t>(result.value));
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
        err::throwFormatError("Format field is missing a closing brace"_el);
    }
    if (next.isSignal()) {
        err::throwFormatError("Format field contains an invalid character"_el);
    }
    const auto character = next;
    if (character == U'{') {
        err::throwFormatError("Format field contains an unexpected opening brace"_el);
    }
    if (!character.isAscii()) {
        err::throwFormatError("Format field specification must use ASCII characters"_el);
    }
    return character;
}

auto FormatParser::readLimitedDecimal(const StringLiteral &tooLargeMessage) -> unit::CpLength {
    auto options = IntegerParseOptions::parserDefault();
    options.setFixedBase(IntegerBase::Decimal).setMaximumDigits(cMaximumDecimalDigits);
    const auto result = _reader.parseInteger(options);
    if (result.status == ReadNumberStatus::NoDigits) {
        return unit::CpLength::zero();
    }
    if (result.status != ReadNumberStatus::Success || result.value > cMaximumFieldWidth.toRawValue()) {
        err::throwFormatError(tooLargeMessage);
    }
    return unit::CpLength::fromSizeT(static_cast<std::size_t>(result.value));
}

auto FormatParser::parseSpecification() -> FormatSpec {
    auto spec = FormatSpec{};
    if (currentChar() == U'}') {
        consumeChar();
        return spec;
    }
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

void FormatParser::parseAlignment(FormatSpec &spec) {
    const auto first = currentChar();
    if (first == U'<' || first == U'>' || first == U'^') {
        consumeSpecificationChar();
        spec.alignment = first == U'<' ? bgeo::AlignmentFlag::Left
            : first == U'>'            ? bgeo::AlignmentFlag::Right
                                       : bgeo::AlignmentFlag::HCenter;
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
        spec.alignment = second == U'<' ? bgeo::AlignmentFlag::Left
            : second == U'>'            ? bgeo::AlignmentFlag::Right
                                        : bgeo::AlignmentFlag::HCenter;
        return;
    }
    _reader.restore(state);
}

void FormatParser::parseSign(FormatSpec &spec) {
    if (_reader.advanceIf(U'+')) {
        spec.signMode = IntegerSignMode::Always;
    } else if (_reader.advanceIf(U' ')) {
        spec.signMode = IntegerSignMode::Space;
    } else if (_reader.advanceIf(U'-')) {
        spec.signMode = IntegerSignMode::NegativeOnly;
    }
}

void FormatParser::parseWidth(FormatSpec &spec) {
    const auto width = readLimitedDecimal("Format field width is too large"_el);
    if (!width.isZero()) {
        spec.width = width;
    }
}

void FormatParser::parsePrecision(FormatSpec &spec) {
    if (!_reader.advanceIf(U'.')) {
        return;
    }
    auto options = IntegerParseOptions::parserDefault();
    options.setFixedBase(IntegerBase::Decimal).setMaximumDigits(cMaximumDecimalDigits);
    const auto result = _reader.parseInteger(options);
    if (result.status == ReadNumberStatus::NoDigits) {
        err::throwFormatError("Format field precision is missing digits"_el);
    }
    if (result.status != ReadNumberStatus::Success || result.value > cMaximumFieldWidth.toRawValue()) {
        err::throwFormatError("Format field precision is too large"_el);
    }
    spec.precision = unit::CpLength::fromSizeT(static_cast<std::size_t>(result.value));
}

void FormatParser::parsePresentation(FormatSpec &spec) {
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
        spec.presentation = FormatPresentation::String;
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
        err::throwFormatError("Format field type is not supported"_el);
    }
    if (currentChar() != U'}') {
        err::throwFormatError("Format field specification contains trailing characters"_el);
    }
    consumeSpecificationChar();
}

void FormatParser::parseEscapedPresentation(FormatSpec &spec) {
    consumeSpecificationChar(); // '/'
    auto formatName = String{};
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
        err::throwFormatError("Format escape modifier is not supported"_el);
    }
    spec.presentation = FormatPresentation::EscapedText;
    spec.escapeFormat = format.value();
    spec.escapeAmount = amount;
}

auto FormatParser::resolveArgumentIndex(const std::optional<unit::ArgumentIndex> explicitIndex) -> unit::ArgumentIndex {
    if (explicitIndex.has_value()) {
        if (_indexMode == IndexMode::Automatic) {
            err::throwFormatError("Format pattern mixes automatic and manual argument indexes"_el);
        }
        _indexMode = IndexMode::Manual;
        return explicitIndex.value();
    }
    if (_indexMode == IndexMode::Manual) {
        err::throwFormatError("Format pattern mixes automatic and manual argument indexes"_el);
    }
    _indexMode = IndexMode::Automatic;
    const auto result = _nextAutomaticIndex;
    ++_nextAutomaticIndex;
    return result;
}

void FormatParser::markArgumentIndex(const unit::ArgumentIndex argumentIndex) {
    if (!argumentIndex.isWithin(cMaximumFields)) {
        err::throwFormatError("Format argument index is too large"_el);
    }
    const auto argumentIndexValue = argumentIndex.toSizeT();
    if (argumentIndexValue >= _data->usedArguments.size()) {
        _data->usedArguments.resize(argumentIndexValue + 1U, false);
    }
    _data->usedArguments[argumentIndexValue] = true;
    _data->argumentCount =
        std::max(_data->argumentCount, argumentIndex.distanceFromZero() + unit::ArgumentCount::one());
}

void FormatParser::parseField() {
    requireFieldLimit();
    const auto argumentIndex = resolveArgumentIndex(readIndex());

    const auto separator = consumeChar();
    if (separator.isEndOfData()) {
        err::throwFormatError("Format field is missing a closing brace"_el);
    }
    if (separator.isSignal()) {
        err::throwFormatError("Format field contains an invalid character"_el);
    }
    auto spec = FormatSpec{};
    if (separator == U':') {
        spec = parseSpecification();
    } else if (separator != U'}') {
        err::throwFormatError("Format field contains unexpected characters after argument index"_el);
    }

    _data->parts.emplace_back(FormatPart::fromField(spec, argumentIndex));
    markArgumentIndex(argumentIndex);
    ++_data->fieldCount;
}

auto FormatParser::finish() -> FormatDataPtr {
    flushStaticText();
    for (
        auto argumentIndex = unit::ArgumentIndex::zero(); argumentIndex.toSizeT() < _data->usedArguments.size();
        ++argumentIndex) {
        if (!_data->usedArguments[argumentIndex.toSizeT()]) {
            err::throwFormatError("Manual format argument indexes must be contiguous from zero"_el);
        }
    }
    return std::move(_data);
}

}
