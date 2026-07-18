// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "JsonEscapeFormatter.hpp"

#include "../IntegerFormat.hpp"
#include "../Literals.hpp"

#include <cstddef>

namespace erbsland::text::impl {

using namespace text::literals;

auto JsonEscapeFormatter::unicodeEscapeFormat(const std::size_t width) noexcept -> IntegerFormat {
    return IntegerFormat{IntegerBase::Hexadecimal}
        .setFlags(IntegerFormatFlag::ZeroFill)
        .setLetterCase(LetterCase::Uppercase)
        .setFieldWidth(unit::CpLength::fromSizeT(width));
}

auto JsonEscapeFormatter::needsEscape(const Char character, const EscapeAmount amount) const noexcept -> bool {
    const auto required = character == U'"' || character == U'\\' || character.toRawValue() <= 0x1FU;
    return needsEscapeByAmount(character, amount, required, false);
}

void JsonEscapeFormatter::escape(const Char character, AnyStringBuilder &builder) const {
    switch (character.toRawValue()) {
    case U'"':
        builder.append("\\\""_el);
        return;
    case U'\\':
        builder.append("\\\\"_el);
        return;
    case U'\b':
        builder.append("\\b"_el);
        return;
    case U'\f':
        builder.append("\\f"_el);
        return;
    case U'\n':
        builder.append("\\n"_el);
        return;
    case U'\r':
        builder.append("\\r"_el);
        return;
    case U'\t':
        builder.append("\\t"_el);
        return;
    default:
        break;
    }
    appendUnicodeEscape(builder, static_cast<uint32_t>(character.toRawValue()));
}

auto JsonEscapeFormatter::escapeSize(const Char character, [[maybe_unused]] const StringKind stringKind) const noexcept
    -> std::size_t {
    switch (character.toRawValue()) {
    case U'"':
    case U'\\':
    case U'\b':
    case U'\f':
    case U'\n':
    case U'\r':
    case U'\t':
        return 2U;
    default:
        break;
    }
    return character.toRawValue() <= 0xFFFFU ? 6U : 12U;
}

auto JsonEscapeFormatter::instance() noexcept -> const EscapeFormatterPtr & {
    static EscapeFormatterPtr result = std::make_shared<JsonEscapeFormatter>();
    return result;
}

void JsonEscapeFormatter::appendUnicodeEscape(AnyStringBuilder &builder, const uint32_t codePoint) {
    if (codePoint <= 0xFFFFU) {
        builder.append("\\u"_el);
        builder.appendInteger(codePoint, unicodeEscapeFormat(4U));
        return;
    }
    const auto value = codePoint - 0x10000U;
    const auto highSurrogate = 0xD800U + (value >> 10U);
    const auto lowSurrogate = 0xDC00U + (value & 0x3FFU);
    builder.append("\\u"_el);
    builder.appendInteger(highSurrogate, unicodeEscapeFormat(4U));
    builder.append("\\u"_el);
    builder.appendInteger(lowSurrogate, unicodeEscapeFormat(4U));
}

}
