// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FormatWriter.hpp"

#include "ThrowHelper.hpp"

#include "../Literals.hpp"
#include "../StringConverter.hpp"
#include "../u16/U16StringEditor.hpp"
#include "../u32/U32StringEditor.hpp"
#include "../u8/U8String.hpp"
#include "../u8/U8StringLiteral.hpp"

namespace erbsland::text::impl {

using namespace erbsland::text::literals;

using bgeo::Alignment;
using bgeo::AlignmentFlag;

FormatWriter::FormatWriter(AnyStringBuilder &builder) : _builder{builder} {
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

auto FormatWriter::defaultFieldText(const FormatArgument &argument, const FormatSpec &spec) -> U8StringEditor {
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
    text::impl::throwFormatError("Unsupported format argument"_el);
}

auto FormatWriter::textFieldText(const FormatArgument &argument, const FormatSpec &spec) -> U8StringEditor {
    requireTextCompatibleSpec(spec);

    auto text = U8StringEditor{};
    switch (argument.kind()) {
    case FormatArgumentKind::U8Text:
        text = U8StringEditor{argument.u8Text()};
        break;
    case FormatArgumentKind::U16Text:
        text = U8StringEditor{StringConverter{argument.u16Text()}.toU8String()};
        break;
    case FormatArgumentKind::U32Text:
        text = U8StringEditor{StringConverter{argument.u32Text()}.toU8String()};
        break;
    case FormatArgumentKind::Boolean:
        text = U8StringEditor::fromBoolean(argument.boolean());
        break;
    case FormatArgumentKind::Character:
        text = characterText(argument.character());
        break;
    default:
        text::impl::throwFormatError("Format field requires a text argument"_el);
    }

    return applyLayout(applyPrecision(text, spec), spec, AlignmentFlag::Left);
}

auto FormatWriter::integerFieldText(const FormatArgument &argument, const FormatSpec &spec) -> U8StringEditor {
    switch (argument.kind()) {
    case FormatArgumentKind::SignedInteger:
        return applyLayout(integerFieldText(argument.signedInteger(), spec), spec, AlignmentFlag::Right);
    case FormatArgumentKind::UnsignedInteger:
        return applyLayout(integerFieldText(argument.unsignedInteger(), spec), spec, AlignmentFlag::Right);
    default:
        text::impl::throwFormatError("Format field requires an integer argument"_el);
    }
}

auto FormatWriter::floatFieldText(const FormatArgument &argument, const FormatSpec &spec) -> U8StringEditor {
    if (spec.alternateForm) {
        text::impl::throwFormatError("Alternate floating point format is not supported"_el);
    }
    if (argument.kind() != FormatArgumentKind::FloatingPoint) {
        text::impl::throwFormatError("Format field requires a floating point argument"_el);
    }

    auto text = U8StringEditor{String::fromFloat(argument.floatingPoint(), floatFormat(spec))};
    if (spec.signMode == IntegerSignMode::Always && !text.startsWith("-"_el)) {
        text = U8StringEditor{String::fromJoined({"+"_el, text})};
    } else if (spec.signMode == IntegerSignMode::Space && !text.startsWith("-"_el)) {
        text = U8StringEditor{String::fromJoined({" "_el, text})};
    }
    return applyLayout(std::move(text), spec, AlignmentFlag::Right);
}

auto FormatWriter::escapedFieldText(const FormatArgument &argument, const FormatSpec &spec) -> U8StringEditor {
    if (spec.signMode != IntegerSignMode::NegativeOnly || spec.alternateForm || spec.zeroFill) {
        text::impl::throwFormatError("Escaped text format does not support numeric modifiers"_el);
    }

    switch (argument.kind()) {
    case FormatArgumentKind::U8Text: {
        auto text = U8StringEditor{argument.u8Text()};
        if (spec.precision.has_value()) {
            text.truncate(spec.precision.value());
        }
        return applyLayout(text.toEscaped(spec.escapeFormat, spec.escapeAmount), spec, AlignmentFlag::Left);
    }
    case FormatArgumentKind::U16Text: {
        auto text = argument.u16Text();
        if (spec.precision.has_value()) {
            text = text.truncated(spec.precision.value());
        }
        return applyLayout(
            U8StringEditor{StringConverter{text.toEscaped(spec.escapeFormat, spec.escapeAmount)}.toU8String()},
            spec,
            AlignmentFlag::Left);
    }
    case FormatArgumentKind::U32Text: {
        auto text = U32StringEditor{argument.u32Text()};
        if (spec.precision.has_value()) {
            text.truncate(spec.precision.value());
        }
        return applyLayout(
            U8StringEditor{StringConverter{text.toEscaped(spec.escapeFormat, spec.escapeAmount)}.toU8String()},
            spec,
            AlignmentFlag::Left);
    }
    case FormatArgumentKind::Boolean:
        return applyLayout(U8StringEditor::fromBoolean(argument.boolean()), spec, AlignmentFlag::Left);
    case FormatArgumentKind::Character: {
        auto text = characterText(argument.character());
        if (spec.precision.has_value()) {
            text.truncate(spec.precision.value());
        }
        return applyLayout(text.toEscaped(spec.escapeFormat, spec.escapeAmount), spec, AlignmentFlag::Left);
    }
    default:
        text::impl::throwFormatError("Escaped format field requires a text argument"_el);
    }
}

auto FormatWriter::characterText(const Char character) -> U8StringEditor {
    return U8StringEditor::fromCharacter(character);
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
    return presentation == FormatPresentation::StringEditor;
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
        text::impl::throwFormatError("Format field requires an integer presentation"_el);
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
        text::impl::throwFormatError("Format field requires a floating point presentation"_el);
    }
    result.setLetterCase(spec.letterCase);
    if (spec.precision.has_value()) {
        result.setPrecision(unit::ElementCount{spec.precision.value().toRawValue()});
    }
    return result;
}

auto FormatWriter::formattedAlignment(const FormatSpec &spec, const AlignmentFlag defaultAlignment) -> Alignment {
    const auto alignment = spec.alignment == AlignmentFlag::None ? defaultAlignment : spec.alignment;
    return Alignment{alignment};
}

auto FormatWriter::zeroPaddedNumericText(const String &text, const unit::CpLength width) -> U8StringEditor {
    static const auto signPrefixCharSet = CharSet{U'-', U'+', U' '};
    static const auto basePrefixCharSet = CharSet{U'x', U'X', U'b', U'B', U'o', U'O'};
    const auto textLength = text.characterLength();
    if (textLength >= width) {
        return U8StringEditor{text};
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

    const auto prefixEnd = text.indexAt(cpPrefixEnd);
    return U8StringEditor{String::fromJoined(
        {text.slice(unit::ByteRange{unit::ByteIndex::zero(), prefixEnd}),
            String::fromCharacter(U'0', width - textLength),
            text.slice(unit::ByteRange{prefixEnd, text.indexAt(StringSide::Back)})})};
}

auto FormatWriter::applyPrecision(const String &text, const FormatSpec &spec) -> StringEditor {
    if (!spec.precision.has_value()) {
        return StringEditor{text};
    }
    return StringEditor{text.truncated(spec.precision.value())};
}

auto FormatWriter::applyLayout(U8StringEditor text, const FormatSpec &spec, const AlignmentFlag defaultAlignment)
    -> StringEditor {
    if (!spec.width.has_value()) {
        return text;
    }
    if (spec.zeroFill && spec.fill == U' ' &&
        (spec.alignment == AlignmentFlag::None || spec.alignment == AlignmentFlag::Right) &&
        defaultAlignment == AlignmentFlag::Right) {
        return zeroPaddedNumericText(text, spec.width.value());
    }
    return text.aligned(spec.width.value(), formattedAlignment(spec, defaultAlignment), spec.fill);
}

void FormatWriter::requireTextCompatibleSpec(const FormatSpec &spec) {
    if (spec.signMode != IntegerSignMode::NegativeOnly || spec.alternateForm || spec.zeroFill) {
        text::impl::throwFormatError("Text format does not support numeric modifiers"_el);
    }
}

}
