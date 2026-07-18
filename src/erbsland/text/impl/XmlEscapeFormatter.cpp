// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "XmlEscapeFormatter.hpp"

#include "../IntegerBase.hpp"
#include "../Literals.hpp"

#include <cstdint>

namespace erbsland::text::impl {

using namespace text::literals;

auto XmlEscapeFormatter::needsEscape(const Char character, const EscapeAmount amount) const noexcept -> bool {
    const auto required = character == U'&' || character == U'<' || character == U'>';
    const auto balanced = character == U'"' || character == U'\'';
    return needsEscapeByAmount(character, amount, required, balanced);
}

void XmlEscapeFormatter::escape(const Char character, AnyStringBuilder &builder) const {
    switch (character.toRawValue()) {
    case U'&':
        builder.append("&amp;"_el);
        return;
    case U'<':
        builder.append("&lt;"_el);
        return;
    case U'>':
        builder.append("&gt;"_el);
        return;
    case U'"':
        builder.append("&quot;"_el);
        return;
    case U'\'':
        builder.append("&apos;"_el);
        return;
    default:
        break;
    }
    builder.append("&#"_el);
    builder.appendInteger(static_cast<uint32_t>(character.toRawValue()));
    builder.append(U';');
}

auto XmlEscapeFormatter::escapeSize(const Char character, [[maybe_unused]] const StringKind stringKind) const noexcept
    -> std::size_t {
    switch (character.toRawValue()) {
    case U'<':
    case U'>':
        return 4U;
    case U'&':
        return 5U;
    case U'"':
    case U'\'':
        return 6U;
    default:
        return IntegerBase{IntegerBase::Decimal}.digitCount(static_cast<uint32_t>(character.toRawValue())) + 3U;
    }
}

auto XmlEscapeFormatter::instance() noexcept -> const EscapeFormatterPtr & {
    static EscapeFormatterPtr result = std::make_shared<XmlEscapeFormatter>();
    return result;
}

}
