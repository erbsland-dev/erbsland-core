// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "MarkdownEscapeFormatter.hpp"

#include "../IntegerBase.hpp"
#include "../Literals.hpp"

#include <cstdint>

namespace erbsland::text::impl {

using namespace text::literals;

auto MarkdownEscapeFormatter::needsEscape(const Char character, const EscapeAmount amount) const noexcept -> bool {
    const auto value = character.toRawValue();
    const auto required = value >= U'!' && value <= U'~' && !character.isAsciiAlphanumeric();
    return needsEscapeByAmount(character, amount, required, false);
}

void MarkdownEscapeFormatter::escape(const Char character, AnyStringBuilder &builder) const {
    const auto value = character.toRawValue();
    if (value >= U'!' && value <= U'~' && !character.isAsciiAlphanumeric()) {
        builder.append(U'\\');
        builder.append(character);
        return;
    }
    builder.append("&#"_el);
    builder.appendInteger(static_cast<uint32_t>(value));
    builder.append(U';');
}

auto MarkdownEscapeFormatter::escapeSize(
    const Char character, [[maybe_unused]] const StringKind stringKind) const noexcept -> std::size_t {
    const auto value = character.toRawValue();
    if (value >= U'!' && value <= U'~' && !character.isAsciiAlphanumeric()) {
        return 2U;
    }
    return IntegerBase{IntegerBase::Decimal}.digitCount(static_cast<uint32_t>(value)) + 3U;
}

auto MarkdownEscapeFormatter::instance() noexcept -> const EscapeFormatterPtr & {
    static EscapeFormatterPtr result = std::make_shared<MarkdownEscapeFormatter>();
    return result;
}

}
