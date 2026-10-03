// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AsciiOptionText.hpp"
#include "OptionLiterals.hpp"

#include "../Option.hpp"

#include "../../text/String.hpp"
#include "../../text/StringEditor.hpp"
#include "../../unit/ByteCount.hpp"

namespace erbsland::options::impl {

/// Test whether a detailed-help keyword is a bounded ASCII name or option/choice selector.
/// @param name The requested help keyword, including any option-name prefix.
/// @return Whether every selector component uses its permitted name form.
/// @tested{OptionsParserTest OptionDocumentTest}
[[nodiscard]] inline auto isValidHelpName(const text::String &name) noexcept -> bool {
    if (name.length() > unit::ByteCount{203U}) {
        return false;
    }
    const auto parts = name.splitAtFirst(option_literal::cHelpPathSeparator);
    if (!parts) {
        return Option::isValidOptionName(name) || Option::isValidPositionalName(name);
    }
    const auto &[owner, choice] = *parts;
    return (Option::isValidOptionName(owner) || Option::isValidPositionalName(owner)) &&
        Option::isValidPositionalName(choice);
}

/// Normalize a help selector without changing option prefixes or short-option case.
/// @param name A raw simple name or qualified option/choice selector.
/// @return The canonical selector, or the original input if it is malformed.
/// @tested{OptionHelpDocumentTest}
[[nodiscard]] inline auto normalizeHelpName(const text::String &name) -> text::String {
    if (!isValidHelpName(name)) {
        return name;
    }
    const auto parts = name.splitAtFirst(option_literal::cHelpPathSeparator);
    if (!parts) {
        return normalizeOptionName(name);
    }
    auto result = text::StringEditor{normalizeOptionName(parts->first)};
    result.append(option_literal::cHelpPathSeparator);
    result.append(normalizeOptionName(parts->second));
    return result;
}

}
