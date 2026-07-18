// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RegExEscapeFormatter.hpp"

#include "../IntegerBase.hpp"
#include "../IntegerFormat.hpp"
#include "../Literals.hpp"

#include <cstdint>

namespace erbsland::text::impl {

using namespace text::literals;

auto RegExEscapeFormatter::needsEscape(const Char character, const EscapeAmount amount) const noexcept -> bool {
    return needsEscapeByAmount(character, amount, character.isSpecialRegexCharacter(), false);
}

void RegExEscapeFormatter::escape(const Char character, AnyStringBuilder &builder) const {
    if (character.isSpecialRegexCharacter()) {
        builder.append(U'\\');
        builder.append(character);
        return;
    }
    const auto codePoint = static_cast<uint32_t>(character.toRawValue());
    builder.append("\\x{"_el);
    builder.appendInteger(codePoint, IntegerFormat{IntegerBase::Hexadecimal}.setLetterCase(LetterCase::Uppercase));
    builder.append(U'}');
}

auto RegExEscapeFormatter::escapeSize(const Char character, [[maybe_unused]] const StringKind stringKind) const noexcept
    -> std::size_t {
    if (character.isSpecialRegexCharacter()) {
        return 2U;
    }
    return IntegerBase{IntegerBase::Hexadecimal}.digitCount(static_cast<uint32_t>(character.toRawValue())) + 4U;
}

auto RegExEscapeFormatter::instance() noexcept -> const EscapeFormatterPtr & {
    static EscapeFormatterPtr result = std::make_shared<RegExEscapeFormatter>();
    return result;
}

}
