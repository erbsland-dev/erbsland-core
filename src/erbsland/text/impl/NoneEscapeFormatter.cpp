// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "NoneEscapeFormatter.hpp"

namespace erbsland::text::impl {

auto NoneEscapeFormatter::needsEscape(
    [[maybe_unused]] const Char character, [[maybe_unused]] const EscapeAmount amount) const noexcept -> bool {
    return false;
}

void NoneEscapeFormatter::escape(const Char character, AnyStringBuilder &builder) const {
    builder.append(character);
}

auto NoneEscapeFormatter::escapeSize(const Char character, const StringKind stringKind) const noexcept -> std::size_t {
    return character.encodedSize(stringKind);
}

auto NoneEscapeFormatter::instance() noexcept -> const EscapeFormatterPtr & {
    static EscapeFormatterPtr result = std::make_shared<NoneEscapeFormatter>();
    return result;
}

}
