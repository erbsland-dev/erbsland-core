// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ThrowHelper.hpp"

#include "../Char.hpp"
#include "../Literals.hpp"

#include <array>
#include <compare>
#include <cstddef>
#include <optional>
#include <utility>

namespace erbsland::text::impl {

using namespace literals;

/// Boolean literals for UTF-8 conversion.
/// Uneven index literals are true, even index literals are false.
inline constexpr auto cU8BooleanLiterals = std::array{
    "disabled"_el,
    "enabled"_el,
    "false"_el,
    "true"_el,
    "off"_el,
    "on"_el,
    "no"_el,
    "yes"_el,
};

/// Boolean literals for UTF-16 conversion.
/// Uneven index literals are true, even index literals are false.
inline constexpr auto cU16BooleanLiterals = std::array{
    u"disabled"_el,
    u"enabled"_el,
    u"false"_el,
    u"true"_el,
    u"off"_el,
    u"on"_el,
    u"no"_el,
    u"yes"_el,
};

/// Boolean literals for UTF-32 conversion.
/// Uneven index literals are true, even index literals are false.
inline constexpr auto cU32BooleanLiterals = std::array{
    U"disabled"_el,
    U"enabled"_el,
    U"false"_el,
    U"true"_el,
    U"off"_el,
    U"on"_el,
    U"no"_el,
    U"yes"_el,
};

/// Parse an ELCL boolean literal.
/// @param text The string to parse.
/// @param literals The width-native boolean literal table.
/// @return The parsed value, or no value if the complete text is not a supported literal.
/// @tested{BooleanConversionTest}
template <typename tString, typename tLiterals>
[[nodiscard]] auto parseBoolean(const tString &text, const tLiterals &literals) noexcept -> std::optional<bool> {
    for (std::size_t index = 0; index < literals.size(); ++index) {
        const auto &literal = literals[index];
        if (text.length() == literal.length() &&
            text.compare(literal, Char::compareAsciiFolded) == std::strong_ordering::equal) {
            return (index & 0b1U) == 0b1U; // lowest index bit is the value
        }
    }
    return std::nullopt;
}

/// Parse an ELCL boolean literal, returning a default value on failure.
/// @param text The string to parse.
/// @param literals The width-native boolean literal table.
/// @param defaultValue The value returned for unsupported input.
/// @return The parsed value or `defaultValue`.
/// @tested{BooleanConversionTest}
template <typename tString, typename tLiterals>
[[nodiscard]] auto parseBooleanOrDefault(
    const tString &text, const tLiterals &literals, const bool defaultValue) noexcept -> bool {
    return parseBoolean(text, literals).value_or(defaultValue);
}

/// Parse an ELCL boolean literal or throw.
/// @param text The string to parse.
/// @param literals The width-native boolean literal table.
/// @return The parsed value.
/// @throws err::ParseError if the complete text is not a supported literal.
/// @tested{BooleanConversionTest}
template <typename tString, typename tLiterals>
[[nodiscard]] auto parseBooleanOrThrow(const tString &text, const tLiterals &literals) -> bool {
    if (const auto result = parseBoolean(text, literals)) {
        return *result;
    }
    throwParseError("Boolean text is not a supported literal");
}

}
