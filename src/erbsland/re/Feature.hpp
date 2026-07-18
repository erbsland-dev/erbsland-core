// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/Literals.hpp"
#include "../text/StdFormatForText.hpp"
#include "../text/String.hpp"

#include <cstdint>
#include <format>

namespace erbsland::re {

/// A feature of the regular expression syntax or engine.
enum class Feature : uint16_t {
    // compatibility options
    QuotedLiterals = 1U << 0U,        ///< Using quotes `\\Q...\\E`.
    EscapeBell = 1U << 1U,            ///< The escape sequence `\\a` for bell character.
    EscapeControl = 1U << 2U,         ///< The escape sequence `\\cX` for control character.
    EscapeEscape = 1U << 3U,          ///< The escape sequence `\\e` for the escape character.
    EscapeFormFeed = 1U << 4U,        ///< The escape sequence `\\f` for form feed character.
    EscapeOctal = 1U << 5U,           ///< The escape sequence `\\o{nnn}` for octal character.
    EscapeHex = 1U << 6U,             ///< The escape sequence `\\xnn` and `\\x{nnnn}` for hexadecimal character.
    EscapeLongUnicode = 1U << 7U,     ///< The escape sequence `\\Unnnnnnnn` for 32-bit Unicode character.
    EscapeHorizontalSpace = 1U << 8U, ///< The escape sequence `\\h` for horizontal space character.
    EscapeVerticalSpace = 1U << 9U,   ///< The escape sequence `\\v` for vertical space character.
    PosixClasses = 1U << 10U,         ///< Using POSIX character classes like `[:digit:]`
    AnchorLowercaseZ = 1U << 11U,     ///< Using the anchor `\\z` for the end of the string.
    // pattern safety and pedagogy
    EmptyAlternatives = 1U << 12U, ///< Allow empty alternatives, like `(?:a|b|)`
    EmptyGroups = 1U << 13U,       ///< Allow empty groups, like `()`
    /// Allow null characters in regular expression patterns.
    AcceptNullInPattern = 1U << 14U,
    /// Allow null characters in matching input.
    AcceptNullInInput = 1U << 15U,
    /// All compatibility options.
    AllCompatibility = QuotedLiterals | EscapeBell | EscapeControl | EscapeEscape | EscapeFormFeed | EscapeOctal |
        EscapeHex | EscapeLongUnicode | EscapeHorizontalSpace | EscapeVerticalSpace | PosixClasses | AnchorLowercaseZ,
    /// The default value (enable all compatibility features and null characters in input)
    Default = AllCompatibility | AcceptNullInInput
};

/// Convert a feature to a string representation.
[[nodiscard]] inline auto toString(const Feature feature) -> text::String {
    using namespace text::literals;
    switch (feature) {
    case Feature::QuotedLiterals:
        return "QuotedLiterals"_el;
    case Feature::EscapeBell:
        return "EscapeBell"_el;
    case Feature::EscapeControl:
        return "EscapeControl"_el;
    case Feature::EscapeEscape:
        return "EscapeEscape"_el;
    case Feature::EscapeFormFeed:
        return "EscapeFormFeed"_el;
    case Feature::EscapeOctal:
        return "EscapeOctal"_el;
    case Feature::EscapeHex:
        return "EscapeHex"_el;
    case Feature::EscapeLongUnicode:
        return "EscapeLongUnicode"_el;
    case Feature::EscapeHorizontalSpace:
        return "EscapeHorizontalSpace"_el;
    case Feature::EscapeVerticalSpace:
        return "EscapeVerticalSpace"_el;
    case Feature::PosixClasses:
        return "PosixClasses"_el;
    case Feature::AnchorLowercaseZ:
        return "AnchorLowercaseZ"_el;
    case Feature::EmptyAlternatives:
        return "EmptyAlternatives"_el;
    case Feature::EmptyGroups:
        return "EmptyGroups"_el;
    case Feature::AcceptNullInPattern:
        return "AcceptNullInPattern"_el;
    case Feature::AcceptNullInInput:
        return "AcceptNullInInput"_el;
    default:
        return {};
    }
}

}

template <>
struct std::formatter<erbsland::re::Feature> : std::formatter<erbsland::text::String> {
    auto format(const erbsland::re::Feature feature, std::format_context &ctx) const {
        return std::formatter<erbsland::text::String>::format(erbsland::re::toString(feature), ctx);
    }
};
