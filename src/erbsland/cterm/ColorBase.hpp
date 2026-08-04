// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ColorRole.hpp"

#include "../text/String.hpp"

#include <array>
#include <cstddef>

namespace erbsland::cterm {

/// Shared implementation for foreground and background color values.
/// @tested{ColorParsingTest ColorTest}
class ColorBase {
public:
    /// Internal color identifiers used for ANSI conversion and parsing.
    enum class Value : uint8_t {
        Black = 0,     ///< Black
        Red,           ///< Dark red
        Green,         ///< Dark green
        Yellow,        ///< Dark yellow
        Blue,          ///< Blue
        Magenta,       ///< Magenta
        Cyan,          ///< Cyan
        White,         ///< Light gray
        BrightBlack,   ///< Gray
        BrightRed,     ///< Bright red
        BrightGreen,   ///< Bright green
        BrightYellow,  ///< Bright yellow
        BrightBlue,    ///< Blue
        BrightMagenta, ///< Magenta
        BrightCyan,    ///< Cyan
        BrightWhite,   ///< White
        Default,       ///< The default color of the terminal
        Inherited,     ///< Inherited color from the layer below, or use the default color.
        _Count,        ///< Number of values.
    };

protected:
    /// Create the base color wrapper from one encoded color value.
    constexpr explicit ColorBase(const Value value) : _value{value} {}

    // defaults
    ColorBase() = default;
    ColorBase(const ColorBase &) = default;
    ColorBase(ColorBase &&) = default;
    auto operator=(const ColorBase &) -> ColorBase & = default;
    auto operator=(ColorBase &&) -> ColorBase & = default;

public: // operators
    /// Compare two color base values for equality.
    auto operator==(const ColorBase &) const noexcept -> bool = default;
    /// Compare two color base values for inequality.
    auto operator!=(const ColorBase &) const noexcept -> bool = default;

public: // conversion
    /// Convert the color name to a string.
    [[nodiscard]] auto toString() const -> text::String;

protected:
    /// Create a color enum from the given string, or return a fallback.
    [[nodiscard]] static auto enumFromString(const text::String &str, Value defaultValue) -> Value;
    /// Create a color enum from the given string.
    [[nodiscard]] static auto enumFromStringOrThrow(const text::String &str) -> Value;
    /// Create brighter enum.
    [[nodiscard]] static auto brighterEnum(Value value) -> Value;

protected:
    /// One color table entry with conversion metadata.
    struct TableEntry {
        Value value;              ///< The encoded color value.
        int ansiCode;             ///< The role-relative ANSI color code.
        text::StringLiteral name; ///< The lowercase textual name.
    };
    /// The lookup table for all supported color values.
    using ColorTable = std::array<TableEntry, static_cast<std::size_t>(Value::_Count)>;
    /// Access the lookup table entry for this color value.
    [[nodiscard]] auto tableEntry() const noexcept -> const TableEntry &;
    /// Access the static color lookup table.
    [[nodiscard]] static auto colorTable() noexcept -> const ColorTable &;

protected:
    Value _value{Value::Inherited}; ///< The encoded color value.
};

}
