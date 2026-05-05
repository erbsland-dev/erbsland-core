// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FormatWriter.hpp"

#include "../Literals.hpp"
#include "../StringConverter.hpp"
#include "../u16/U16String.hpp"
#include "../u32/U32String.hpp"
#include "../u8/U8StringLiteral.hpp"
#include "../u8/U8StringView.hpp"

#include "../../err/ThrowHelper.hpp"

namespace erbsland::text::impl {

using namespace erbsland::text::literals;

FormatWriter::FormatWriter(StringBuilder &builder) : _builder{builder} {
}

void FormatWriter::appendField(const FormatPart &part, const FormatArgument &argument) {
    switch (part.kind()) {
    case FormatPartKind::Field:
        _builder.append(defaultFieldText(argument, part.spec()));
        return;
    case FormatPartKind::StaticText:
        _builder.appendAny(part.text());
        return;
    }
}

auto FormatWriter::defaultFieldText(const FormatArgument &argument, const FormatSpec &spec) -> U8String {
    if (spec.presentation == FormatPresentation::EscapedText) {
        return escapedFieldText(argument, spec);
    }
    if (isIntegerPresentation(spec.presentation) ||
        (spec.presentation == FormatPresentation::Default &&
            (argument.kind() == FormatArgumentKind::SignedInteger ||
                argument.kind() == FormatArgumentKind::UnsignedInteger))) {
        return integerFieldText(argument, spec);
    }
    if (isFloatPresentation(spec.presentation) ||
        (spec.presentation == FormatPresentation::Default && argument.kind() == FormatArgumentKind::FloatingPoint)) {
        return floatFieldText(argument, spec);
    }
    if (isTextPresentation(spec.presentation) || spec.presentation == FormatPresentation::Default) {
        return textFieldText(argument, spec);
    }
    err::throwFormatError("Unsupported format argument"_el);
}

auto FormatWriter::textFieldText(const FormatArgument &argument, const FormatSpec &spec) -> U8String {
    requireTextCompatibleSpec(spec);

    auto text = U8String{};
    switch (argument.kind()) {
    case FormatArgumentKind::U8Text:
        text = U8String{argument.u8Text()};
        break;
    case FormatArgumentKind::U16Text:
        text = StringConverter{argument.u16Text()}.toU8String();
        break;
    case FormatArgumentKind::U32Text:
        text = StringConverter{argument.u32Text()}.toU8String();
        break;
    case FormatArgumentKind::Boolean:
        text = U8String::fromBoolean(argument.boolean());
        break;
    case FormatArgumentKind::Character:
        text = characterText(argument.character());
        break;
    default:
        err::throwFormatError("Format field requires a text argument"_el);
    }

    return applyLayout(applyPrecision(text, spec), spec, bgeo::AlignmentFlag::Left);
}

auto FormatWriter::integerFieldText(const FormatArgument &argument, const FormatSpec &spec) -> U8String {
    switch (argument.kind()) {
    case FormatArgumentKind::SignedInteger:
        return applyLayout(integerFieldText(argument.signedInteger(), spec), spec, bgeo::AlignmentFlag::Right);
    case FormatArgumentKind::UnsignedInteger:
        return applyLayout(integerFieldText(argument.unsignedInteger(), spec), spec, bgeo::AlignmentFlag::Right);
    default:
        err::throwFormatError("Format field requires an integer argument"_el);
    }
}

auto FormatWriter::floatFieldText(const FormatArgument &argument, const FormatSpec &spec) -> U8String {
    if (spec.alternateForm) {
        err::throwFormatError("Alternate floating point format is not supported"_el);
    }
    if (argument.kind() != FormatArgumentKind::FloatingPoint) {
        err::throwFormatError("Format field requires a floating point argument"_el);
    }

    auto builder = StringBuilder{};
    builder.appendFloat(argument.floatingPoint(), floatFormat(spec));
    auto text = builder.takeU8String();
    if (spec.signMode == IntegerSignMode::Always && !text.startsWith("-"_el)) {
        auto signedBuilder = StringBuilder{};
        signedBuilder.append(U'+').append(text);
        text = signedBuilder.takeU8String();
    } else if (spec.signMode == IntegerSignMode::Space && !text.startsWith("-"_el)) {
        auto signedBuilder = StringBuilder{};
        signedBuilder.append(U' ').append(text);
        text = signedBuilder.takeU8String();
    }
    return applyLayout(std::move(text), spec, bgeo::AlignmentFlag::Right);
}

auto FormatWriter::escapedFieldText(const FormatArgument &argument, const FormatSpec &spec) -> U8String {
    if (spec.signMode != IntegerSignMode::NegativeOnly || spec.alternateForm || spec.zeroFill) {
        err::throwFormatError("Escaped text format does not support numeric modifiers"_el);
    }

    switch (argument.kind()) {
    case FormatArgumentKind::U8Text: {
        auto text = U8String{argument.u8Text()};
        if (spec.precision.has_value()) {
            text.truncate(spec.precision.value());
        }
        return applyLayout(text.toEscaped(spec.escapeFormat, spec.escapeAmount), spec, bgeo::AlignmentFlag::Left);
    }
    case FormatArgumentKind::U16Text: {
        auto text = argument.u16Text();
        if (spec.precision.has_value()) {
            text = text.truncated(spec.precision.value());
        }
        return applyLayout(
            StringConverter{text.toEscaped(spec.escapeFormat, spec.escapeAmount)}.toU8String(),
            spec,
            bgeo::AlignmentFlag::Left);
    }
    case FormatArgumentKind::U32Text: {
        auto text = U32String{argument.u32Text()};
        if (spec.precision.has_value()) {
            text.truncate(spec.precision.value());
        }
        return applyLayout(
            StringConverter{text.toEscaped(spec.escapeFormat, spec.escapeAmount)}.toU8String(),
            spec,
            bgeo::AlignmentFlag::Left);
    }
    case FormatArgumentKind::Boolean:
        return applyLayout(U8String::fromBoolean(argument.boolean()), spec, bgeo::AlignmentFlag::Left);
    case FormatArgumentKind::Character: {
        auto text = characterText(argument.character());
        if (spec.precision.has_value()) {
            text.truncate(spec.precision.value());
        }
        return applyLayout(text.toEscaped(spec.escapeFormat, spec.escapeAmount), spec, bgeo::AlignmentFlag::Left);
    }
    default:
        err::throwFormatError("Escaped format field requires a text argument"_el);
    }
}

auto FormatWriter::characterText(const Char character) -> U8String {
    return U8String::fromCharacter(character);
}

auto FormatWriter::isIntegerPresentation(const FormatPresentation presentation) noexcept -> bool {
    switch (presentation) {
    case FormatPresentation::Decimal:
    case FormatPresentation::Hex:
    case FormatPresentation::Binary:
    case FormatPresentation::Octal:
        return true;
    default:
        return false;
    }
}

auto FormatWriter::isFloatPresentation(const FormatPresentation presentation) noexcept -> bool {
    switch (presentation) {
    case FormatPresentation::FloatFixed:
    case FormatPresentation::FloatScientific:
    case FormatPresentation::FloatGeneral:
    case FormatPresentation::FloatHex:
        return true;
    default:
        return false;
    }
}

auto FormatWriter::isTextPresentation(const FormatPresentation presentation) noexcept -> bool {
    return presentation == FormatPresentation::String;
}

auto FormatWriter::integerFormat(const FormatSpec &spec) -> IntegerFormat {
    auto result = IntegerFormat::decimal();
    switch (spec.presentation) {
    case FormatPresentation::Default:
    case FormatPresentation::Decimal:
        result.setBase(IntegerBase::Decimal);
        break;
    case FormatPresentation::Hex:
        result.setBase(IntegerBase::Hexadecimal);
        break;
    case FormatPresentation::Binary:
        result.setBase(IntegerBase::Binary);
        break;
    case FormatPresentation::Octal:
        result.setBase(IntegerBase::Octal);
        break;
    default:
        err::throwFormatError("Format field requires an integer presentation"_el);
    }
    result.setLetterCase(spec.letterCase);
    result.setSignMode(spec.signMode);
    if (spec.alternateForm) {
        result.addFlags(IntegerFormatFlag::BasePrefix);
    }
    if (spec.precision.has_value()) {
        result.setPrecision(spec.precision.value());
    }
    return result;
}

auto FormatWriter::floatFormat(const FormatSpec &spec) -> FloatFormat {
    auto result = FloatFormat::defaultFormat();
    switch (spec.presentation) {
    case FormatPresentation::Default:
        break;
    case FormatPresentation::FloatFixed:
        result.setStyle(FloatFormat::Style::Fixed);
        break;
    case FormatPresentation::FloatScientific:
        result.setStyle(FloatFormat::Style::Scientific);
        break;
    case FormatPresentation::FloatGeneral:
        result.setStyle(FloatFormat::Style::General);
        break;
    case FormatPresentation::FloatHex:
        result.setStyle(FloatFormat::Style::Hexadecimal);
        break;
    default:
        err::throwFormatError("Format field requires a floating point presentation"_el);
    }
    result.setLetterCase(spec.letterCase);
    if (spec.precision.has_value()) {
        result.setPrecision(unit::ElementCount{spec.precision.value().toRawValue()});
    }
    return result;
}

auto FormatWriter::formattedAlignment(const FormatSpec &spec, const bgeo::AlignmentFlag defaultAlignment)
    -> bgeo::Alignment {
    const auto alignment = spec.alignment == bgeo::AlignmentFlag::None ? defaultAlignment : spec.alignment;
    return bgeo::Alignment{alignment};
}

auto FormatWriter::zeroPaddedNumericText(const String &text, const unit::CpLength width) -> U8String {
    static const auto signPrefixCharSet = CharSet{U'-', U'+', U' '};
    static const auto basePrefixCharSet = CharSet{U'x', U'X', U'b', U'B', U'o', U'O'};
    const auto textLength = text.characterLength();
    if (textLength >= width) {
        return text;
    }

    auto cpPrefixEnd = unit::CpIndex::zero();
    if (!text.isEmpty()) {
        const auto first = text.charAt(StringSide::Front);
        if (signPrefixCharSet.contains(first)) {
            cpPrefixEnd = unit::CpIndex::one();
        }
    }
    if (textLength >= cpPrefixEnd.distanceFromZero() + unit::CpLength{2U} && text.charAt(cpPrefixEnd) == U'0') {
        const auto basePrefix = text.charAt(cpPrefixEnd + unit::CpLength::one());
        if (basePrefixCharSet.contains(basePrefix)) {
            cpPrefixEnd += unit::CpLength{2U};
        }
    }

    const auto view = U8StringView{text};
    const auto prefixEnd = text.indexAt(cpPrefixEnd);
    auto result = U8String{};
    result.append(view.slice(unit::ByteRange{unit::ByteIndex::zero(), prefixEnd}));
    result.append(U'0', width - textLength);
    result.append(view.slice(unit::ByteRange{prefixEnd, text.indexAt(StringSide::Back)}));
    return result;
}

auto FormatWriter::applyPrecision(const String &text, const FormatSpec &spec) -> String {
    if (!spec.precision.has_value()) {
        return text;
    }
    return text.truncated(spec.precision.value());
}

auto FormatWriter::applyLayout(U8String text, const FormatSpec &spec, const bgeo::AlignmentFlag defaultAlignment)
    -> String {
    if (!spec.width.has_value()) {
        return text;
    }
    if (spec.zeroFill && spec.fill == U' ' &&
        (spec.alignment == bgeo::AlignmentFlag::None || spec.alignment == bgeo::AlignmentFlag::Right) &&
        defaultAlignment == bgeo::AlignmentFlag::Right) {
        return zeroPaddedNumericText(text, spec.width.value());
    }
    return text.aligned(spec.width.value(), formattedAlignment(spec, defaultAlignment), spec.fill);
}

void FormatWriter::requireTextCompatibleSpec(const FormatSpec &spec) {
    if (spec.signMode != IntegerSignMode::NegativeOnly || spec.alternateForm || spec.zeroFill) {
        err::throwFormatError("Text format does not support numeric modifiers"_el);
    }
}

}
