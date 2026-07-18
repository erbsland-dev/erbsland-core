// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/Literals.hpp"
#include "../text/StdFormatForText.hpp"
#include "../text/String.hpp"

#include <cstdint>
#include <format>

namespace erbsland::re {

/// A single flag.
enum class Flag : uint8_t {
    None = 0,
    IgnoreCase = 1U << 0, ///< Ignore case when matching text.
    Multiline = 1U << 1,  ///< Match at the beginning and end of each line.
    DotAll = 1U << 2,     ///< The dot operator also matches newlines.
    Ascii = 1U << 3,      ///< Restrict `\\w`, `\\d`, `\\s` to ASCII only matching.
    Verbose = 1U << 4,    ///< Ignore spacing in the regular expression.

    /// Interpret CRLF line endings like a single LF.
    ///
    /// Both characters of such line-endings are preserved when capturing text.
    /// This works similar to the folding of multi-code point Unicode characters.
    /// E.g. ``a¨`` is interpreted as ``ä``, but both code-points are preserved in text.
    ///
    CRLF = 1U << 5,
};

/// Convert a flag to a string representation.
[[nodiscard]] inline auto toString(const Flag flag) -> text::String {
    using namespace text::literals;
    switch (flag) {
    case Flag::IgnoreCase:
        return "IgnoreCase"_el;
    case Flag::Multiline:
        return "Multiline"_el;
    case Flag::DotAll:
        return "DotAll"_el;
    case Flag::Ascii:
        return "Ascii"_el;
    case Flag::Verbose:
        return "Verbose"_el;
    case Flag::CRLF:
        return "CRLF"_el;
    default:
        return {};
    }
}

}

template <>
struct std::formatter<erbsland::re::Flag> : std::formatter<erbsland::text::String> {
    auto format(const erbsland::re::Flag flag, std::format_context &ctx) const {
        return std::formatter<erbsland::text::String>::format(erbsland::re::toString(flag), ctx);
    }
};
