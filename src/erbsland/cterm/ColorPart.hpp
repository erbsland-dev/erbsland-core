// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ColorRole.hpp"

#include "impl/HashHelper.hpp"

#include "../text/String.hpp"
#include "../text/StringEditor.hpp"

#include <array>
#include <cstddef>
#include <functional>
#include <string_view>

namespace erbsland::cterm {

/// Shared implementation for foreground and background color values.
class ColorBase {
public:
    /// Internal color identifiers used for ANSI conversion and parsing.
    enum class Value : uint8_t {
        Black = 0,     ///< Black
        Red,           ///< Dark red
        Green,         ///< Dark green
        Yellow,        ///< Dark yellow
        Blue,          ///< Dark blue
        Magenta,       ///< Dark magenta
        Cyan,          ///< Dark cyan
        White,         ///< Light gray
        BrightBlack,   ///< Gray
        BrightRed,     ///< Bright red
        BrightGreen,   ///< Bright green
        BrightYellow,  ///< Yellow
        BrightBlue,    ///< Blue
        BrightMagenta, ///< Magenta
        BrightCyan,    ///< Cyan
        BrightWhite,   ///< White
        Default,       ///< The default color of the terminal
        Inherited,     ///< Inherited color from the layer below, or use the default color.

        _Count,
    };

protected:
    ColorBase() = default;
    /// Create the base color wrapper from one encoded color value.
    /// @param value The encoded color value.
    constexpr explicit ColorBase(const Value value) : _value{value} {}

    // defaults
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
    /// Create a color enum from the given string.
    /// @return The color enum.
    /// @throws err::ParameterError if the color does not exist.
    [[nodiscard]] static auto enumFromString(const text::String &str) -> Value;
    /// Create brighter enum
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
    /// @return The matching table entry.
    [[nodiscard]] auto tableEntry() const noexcept -> const TableEntry &;
    /// Access the static color lookup table.
    /// @return The shared color lookup table.
    [[nodiscard]] static auto colorTable() noexcept -> const ColorTable &;

protected:
    Value _value{Value::Inherited};
};

/// A foreground or background color.
template <ColorRole tColorType>
class ColorPart : public ColorBase {
public:
    /// Base ANSI escape code for this color role.
    constexpr static auto cCodeBase = (tColorType == ColorRole::Foreground ? 30 : 40);

public:
    /// Tag type used by the predefined named color constants.
    struct Hue {
        /// The encoded color value.
        Value value;
    };
    /// ANSI black.
    constexpr static auto Black = Hue{Value::Black};
    /// ANSI red.
    constexpr static auto Red = Hue{Value::Red};
    /// ANSI green.
    constexpr static auto Green = Hue{Value::Green};
    /// ANSI yellow.
    constexpr static auto Yellow = Hue{Value::Yellow};
    /// ANSI blue.
    constexpr static auto Blue = Hue{Value::Blue};
    /// ANSI magenta.
    constexpr static auto Magenta = Hue{Value::Magenta};
    /// ANSI cyan.
    constexpr static auto Cyan = Hue{Value::Cyan};
    /// ANSI white / light gray.
    constexpr static auto White = Hue{Value::White};
    /// Bright black / gray.
    constexpr static auto BrightBlack = Hue{Value::BrightBlack};
    /// Bright red.
    constexpr static auto BrightRed = Hue{Value::BrightRed};
    /// Bright green.
    constexpr static auto BrightGreen = Hue{Value::BrightGreen};
    /// Bright yellow.
    constexpr static auto BrightYellow = Hue{Value::BrightYellow};
    /// Bright blue.
    constexpr static auto BrightBlue = Hue{Value::BrightBlue};
    /// Bright magenta.
    constexpr static auto BrightMagenta = Hue{Value::BrightMagenta};
    /// Bright cyan.
    constexpr static auto BrightCyan = Hue{Value::BrightCyan};
    /// Bright white.
    constexpr static auto BrightWhite = Hue{Value::BrightWhite};
    /// Reset this color role to the terminal default.
    constexpr static auto Default = Hue{Value::Default};
    /// Preserve the color from the layer below.
    constexpr static auto Inherited = Hue{Value::Inherited};

public:
    /// Create the inherited color for this role.
    /// The inherited color uses the value from the layer below, or the terminal default if no lower layer exists.
    ColorPart() = default;
    /// Create a color from one of the predefined hue constants.
    /// @param color The named hue.
    constexpr ColorPart(const Hue color) : ColorBase{color.value} {} // NOLINT(*-explicit-constructor)

    // defaults
    ColorPart(const ColorPart &) = default;
    ColorPart(ColorPart &&) = default;
    auto operator=(const ColorPart &) -> ColorPart & = default;
    auto operator=(ColorPart &&) -> ColorPart & = default;

public: // operators
    /// Compare two color parts for equality.
    auto operator==(const ColorPart &) const noexcept -> bool = default;
    /// Compare two color parts for inequality.
    auto operator!=(const ColorPart &) const noexcept -> bool = default;

public: // conversion
    /// Convert this color part to its ANSI SGR numeric code.
    /// @return The numeric ANSI code for this color role.
    [[nodiscard]] auto ansiCode() const noexcept -> int { return tableEntry().ansiCode + cCodeBase; }
    /// Get a hash for this color part.
    /// The color role is included so foreground and background values use distinct hash domains.
    [[nodiscard]] constexpr auto hash() const noexcept -> std::size_t {
        return impl::hashCreate(static_cast<uint8_t>(tColorType), static_cast<uint8_t>(_value));
    }
    /// Create the bright variant of this color.
    /// @return The brightened color, or the unchanged color if no brighter variant exists.
    [[nodiscard]] auto brighter() const noexcept -> ColorPart { return ColorPart{Hue{brighterEnum(_value)}}; }

public: // tools
    /// Create a color from the given string.
    /// Accepts lowercase names as well as identifier-normalized space/underscore variants.
    /// @param str The color name.
    /// @return The parsed color.
    /// @throws err::ParameterError if the color does not exist.
    [[nodiscard]] static auto fromString(const text::String &str) -> ColorPart {
        return ColorPart{Hue{enumFromString(str)}};
    }
    /// Create a color from the given index.
    /// @param index The index from 0 (black) to 15 (bright white), <0 = inherited, 16+ = default
    /// @return The color.
    [[nodiscard]] static auto fromIndex16(const int index) -> ColorPart {
        if (index < 0) {
            return ColorPart{Hue{Value::Inherited}};
        }
        if (index > 15) {
            return ColorPart{Hue{Value::Default}};
        }
        return ColorPart{Hue{static_cast<Value>(index)}};
    }

    /// Access the eight non-bright base colors in ANSI order.
    /// Use with `.brighter()` to get the bright color variants.
    [[nodiscard]] static auto allBaseColors() noexcept -> std::array<ColorPart, 8> {
        return {Black, Red, Green, Yellow, Blue, Magenta, Cyan, White};
    }
};

/// The background color.
using Background = ColorPart<ColorRole::Background>;
/// Short alias for `Background`.
using bg = Background;

/// The foreground color.
using Foreground = ColorPart<ColorRole::Foreground>;
/// Short alias for `Foreground`.
using fg = Foreground;

}

template <>
struct std::hash<erbsland::cterm::Foreground> {
    auto operator()(const erbsland::cterm::Foreground &color) const noexcept -> std::size_t { return color.hash(); }
};

template <>
struct std::hash<erbsland::cterm::Background> {
    auto operator()(const erbsland::cterm::Background &color) const noexcept -> std::size_t { return color.hash(); }
};
