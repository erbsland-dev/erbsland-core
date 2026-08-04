// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ColorBase.hpp"

#include "impl/HashHelper.hpp"

#include <array>
#include <functional>

namespace erbsland::cterm {

/// A foreground or background color.
/// @tested{ColorParsingTest ColorTest}
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
    [[nodiscard]] auto ansiCode() const noexcept -> int { return tableEntry().ansiCode + cCodeBase; }
    /// Get a hash for this color part.
    [[nodiscard]] constexpr auto hash() const noexcept -> std::size_t {
        return impl::hashCreate(static_cast<uint8_t>(tColorType), static_cast<uint8_t>(_value));
    }
    /// Create the bright variant of this color.
    [[nodiscard]] auto brighter() const noexcept -> ColorPart { return ColorPart{Hue{brighterEnum(_value)}}; }

public: // tools
    /// Create a color from the given string, or return a fallback.
    [[nodiscard]] static auto fromString(const text::String &str, const ColorPart defaultValue) -> ColorPart {
        return ColorPart{Hue{enumFromString(str, defaultValue._value)}};
    }
    /// Create a color from the given string.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &str) -> ColorPart {
        return ColorPart{Hue{enumFromStringOrThrow(str)}};
    }
    /// Create a color from the given index.
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
