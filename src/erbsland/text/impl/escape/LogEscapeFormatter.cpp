// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LogEscapeFormatter.hpp"

#include "ConfigEscapeFormatter.hpp"

#include <memory>

namespace erbsland::text::impl {

auto LogEscapeFormatter::needsEscape(const Char character, const EscapeAmount amount) const noexcept -> bool {
    if (character == U'\n') {
        return false;
    }
    return needsEscapeByAmount(character, amount, character.isControlOrFormat(), false);
}

void LogEscapeFormatter::escape(const Char character, AnyStringBuilder &builder) const {
    ConfigEscapeFormatter::regularInstance()->escape(character, builder);
}

auto LogEscapeFormatter::escapeSize(const Char character, const StringKind stringKind) const noexcept -> std::size_t {
    return ConfigEscapeFormatter::regularInstance()->escapeSize(character, stringKind);
}

auto LogEscapeFormatter::instance() noexcept -> const EscapeFormatterPtr & {
    static EscapeFormatterPtr result = std::make_shared<LogEscapeFormatter>();
    return result;
}

}
