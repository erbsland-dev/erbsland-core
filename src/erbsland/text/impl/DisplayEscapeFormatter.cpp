// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DisplayEscapeFormatter.hpp"

#include "CppEscapeFormatter.hpp"

#include <memory>

namespace erbsland::text::impl {

auto DisplayEscapeFormatter::needsEscape(const Char character, const EscapeAmount amount) const noexcept -> bool {
    return needsEscapeByAmount(character, amount, character.isControlOrFormat(), false);
}

void DisplayEscapeFormatter::escape(const Char character, StringBuilder &builder) const {
    CppEscapeFormatter::instance()->escape(character, builder);
}

auto DisplayEscapeFormatter::escapeSize(const Char character, const StringKind stringKind) const noexcept
    -> std::size_t {
    return CppEscapeFormatter::instance()->escapeSize(character, stringKind);
}

auto DisplayEscapeFormatter::instance() noexcept -> const EscapeFormatterPtr & {
    static EscapeFormatterPtr result = std::make_shared<DisplayEscapeFormatter>();
    return result;
}

}
