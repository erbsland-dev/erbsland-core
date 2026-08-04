// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ColorPart.hpp"

#include "../text/String.hpp"

#include <functional>
#include <string_view>
#include <vector>

namespace erbsland::cterm {

/// A foreground/background color pair for terminal rendering.
/// @tested{ColorParsingTest ColorTest}
class Color final {
public: // ctors/dtor/assign/move
    /// Create a color that inherits both components from the layer below.
    /// If no lower layer exists, terminal output resolves inherited colors to the terminal defaults.
    Color() = default;
    /// Create a color from explicit foreground and background parts.
    /// @param foreground The foreground color.
    /// @param background The background color.
    constexpr Color(const Foreground foreground, const Background background) noexcept :
        _foreground{foreground}, _background{background} {}
    /// Create a color with an explicit foreground and inherited background color.
    /// @param foreground The background color.
    constexpr Color(const Foreground foreground) noexcept : _foreground{foreground} {}
    /// @overload
    constexpr Color(const Foreground::Hue foreground) noexcept : _foreground{foreground} {}
    /// Create a color with an explicit background and inherited foreground color.
    /// @param background The background color.
    constexpr Color(const Background background) noexcept : _background{background} {}
    /// @overload
    constexpr Color(const Background::Hue background) noexcept : _background{background} {}
    // defaults
    ~Color() = default;
    Color(const Color &) noexcept = default;
    auto operator=(const Color &) noexcept -> Color & = default;

public: // operators
    /// Compare two colors for equality.
    auto operator==(const Color &other) const -> bool = default;
    /// Compare two colors for inequality.
    auto operator!=(const Color &other) const -> bool = default;

public: // accessors
    /// Get the foreground color.
    [[nodiscard]] auto fg() const noexcept -> Foreground { return _foreground; }
    /// Set the foreground color.
    /// @param foreground The new foreground color.
    void setFg(const Foreground foreground) noexcept { _foreground = foreground; }
    /// Get the background color.
    [[nodiscard]] auto bg() const noexcept -> Background { return _background; }
    /// Set the background color.
    /// @param background The new background color.
    void setBg(const Background background) noexcept { _background = background; }

public: // conversion and tools
    /// Convert this color to its canonical textual representation.
    /// @return The canonical color specification.
    [[nodiscard]] auto toString() const -> text::String;
    /// Combine this color with a new overlay color.
    /// Foreground or background components set to `Inherited` keep the value from this color.
    /// Components set to `Default` explicitly reset to the terminal default color.
    ///
    /// Examples:
    ///
    /// - [this.fg = red] + [new.fg = inherited] => [result.fg = red]
    /// - [this.fg = red] + [new.fg = default] => [result.fg = default]
    /// - [this.fg = red] + [new.fg = green] => [result.fg = green]
    ///
    /// @param overlay The overlay color.
    /// @return The new color with the overlay applied.
    [[nodiscard]] auto overlayWith(const Color &overlay) const -> Color;
    /// Convert a color or color-pair into a block color, or return a fallback.
    /// @param str The textual color specification.
    /// @param defaultValue The value returned for invalid text.
    /// @return The parsed color or `defaultValue`.
    [[nodiscard]] static auto fromString(const text::String &str, Color defaultValue) -> Color;
    /// Convert a color or color-pair into a block color.
    /// @param str The textual color specification.
    /// @return The parsed color.
    /// @throws err::ParseError if the specification is invalid.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &str) -> Color;
    /// Converts two indexes into a color-pair.
    /// @see ColorPart::fromIndex16 for details.
    [[nodiscard]] static auto fromIndex16(const int fgIndex, const int bgIndex) -> Color {
        return Color{Foreground::fromIndex16(fgIndex), Background::fromIndex16(bgIndex)};
    }
    /// Shortcut to set fg and bg to default.
    [[nodiscard]] constexpr static auto reset() noexcept -> Color {
        return Color{Foreground::Default, Background::Default};
    }
    /// Get a hash for this color pair.
    [[nodiscard]] constexpr auto hash() const noexcept -> std::size_t {
        return impl::hashCreate(_foreground.hash(), _background.hash());
    }

public: // data
    /// Foreground component of the color.
    Foreground _foreground;
    /// Background component of the color.
    Background _background;
};

/// A list of terminal colors.
using ColorList = std::vector<Color>;

}

template <>
struct std::hash<erbsland::cterm::Color> {
    auto operator()(const erbsland::cterm::Color &color) const noexcept -> std::size_t { return color.hash(); }
};
