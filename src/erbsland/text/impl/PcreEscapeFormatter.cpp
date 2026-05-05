// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PcreEscapeFormatter.hpp"

#include "../IntegerBase.hpp"
#include "../IntegerFormat.hpp"
#include "../Literals.hpp"

#include <cstdint>

namespace erbsland::text::impl {

using namespace text::literals;

auto PcreEscapeFormatter::needsEscape(const Char character, const EscapeAmount amount) const noexcept -> bool {
    return needsEscapeByAmount(character, amount, isMetacharacter(character), false);
}

void PcreEscapeFormatter::escape(const Char character, StringBuilder &builder) const {
    if (isMetacharacter(character)) {
        builder.append(U'\\');
        builder.append(character);
        return;
    }
    const auto codePoint = static_cast<uint32_t>(character.toRawValue());
    builder.append("\\x{"_el);
    builder.appendInteger(codePoint, IntegerFormat{IntegerBase::Hexadecimal}.setLetterCase(LetterCase::Uppercase));
    builder.append(U'}');
}

auto PcreEscapeFormatter::escapeSize(const Char character, [[maybe_unused]] const StringKind stringKind) const noexcept
    -> std::size_t {
    if (isMetacharacter(character)) {
        return 2U;
    }
    return IntegerBase{IntegerBase::Hexadecimal}.digitCount(static_cast<uint32_t>(character.toRawValue())) + 4U;
}

auto PcreEscapeFormatter::instance() noexcept -> const EscapeFormatterPtr & {
    static EscapeFormatterPtr result = std::make_shared<PcreEscapeFormatter>();
    return result;
}

auto PcreEscapeFormatter::isMetacharacter(const Char character) noexcept -> bool {
    switch (character.toRawValue()) {
    case U'\\':
    case U'^':
    case U'$':
    case U'.':
    case U'|':
    case U'?':
    case U'*':
    case U'+':
    case U'(':
    case U')':
    case U'[':
    case U']':
    case U'{':
    case U'}':
        return true;
    default:
        return false;
    }
}

}
