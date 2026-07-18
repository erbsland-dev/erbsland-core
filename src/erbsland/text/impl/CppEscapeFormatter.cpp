// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CppEscapeFormatter.hpp"

#include "../IntegerFormat.hpp"
#include "../Literals.hpp"

#include <cstddef>
#include <cstdint>

namespace erbsland::text::impl {

using namespace text::literals;

auto CppEscapeFormatter::integerEscapeFormat(const IntegerBase base, const std::size_t width) noexcept
    -> IntegerFormat {
    return IntegerFormat{base}
        .setFlags(IntegerFormatFlag::ZeroFill)
        .setLetterCase(LetterCase::Uppercase)
        .setFieldWidth(unit::CpLength::fromSizeT(width));
}

auto CppEscapeFormatter::needsEscape(const Char character, const EscapeAmount amount) const noexcept -> bool {
    const auto required = character == U'"' || character == U'\\' || character.isAsciiControl();
    return needsEscapeByAmount(character, amount, required, false);
}

void CppEscapeFormatter::escape(const Char character, AnyStringBuilder &builder) const {
    switch (character.toRawValue()) {
    case U'"':
        builder.append("\\\""_el);
        return;
    case U'\\':
        builder.append("\\\\"_el);
        return;
    case U'\a':
        builder.append("\\a"_el);
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
    case U'\v':
        builder.append("\\v"_el);
        return;
    default:
        break;
    }

    const auto codePoint = static_cast<uint32_t>(character.toRawValue());
    if (character.isAscii()) {
        builder.append(U'\\');
        builder.appendInteger(codePoint, integerEscapeFormat(IntegerBase::Octal, 3U));
        return;
    }
    if (codePoint <= 0xFFFFU) {
        builder.append("\\u"_el);
        builder.appendInteger(codePoint, integerEscapeFormat(IntegerBase::Hexadecimal, 4U));
        return;
    }
    builder.append("\\U"_el);
    builder.appendInteger(codePoint, integerEscapeFormat(IntegerBase::Hexadecimal, 8U));
}

auto CppEscapeFormatter::escapeSize(const Char character, [[maybe_unused]] const StringKind stringKind) const noexcept
    -> std::size_t {
    switch (character.toRawValue()) {
    case U'"':
    case U'\\':
    case U'\a':
    case U'\b':
    case U'\f':
    case U'\n':
    case U'\r':
    case U'\t':
    case U'\v':
        return 2U;
    default:
        break;
    }
    if (character.isAscii()) {
        return 4U;
    }
    return character.toRawValue() <= 0xFFFFU ? 6U : 10U;
}

auto CppEscapeFormatter::instance() noexcept -> const EscapeFormatterPtr & {
    static EscapeFormatterPtr result = std::make_shared<CppEscapeFormatter>();
    return result;
}

}
