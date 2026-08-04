// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Block.hpp"
#include "Color.hpp"
#include "FrameBorder_fwd.hpp"
#include "FrameBorderElement.hpp"
#include "FrameStyle.hpp"

#include "../bgeo/BlockAnchor.hpp"
#include "../text/Char.hpp"

#include <array>
#include <cstdint>

namespace erbsland::cterm {

/// Styling for all line groups of a grid frame.
///
/// The default border draws no lines. Use the all-style constructor or `set()` to enable individual line groups.
/// @tested{FrameBorderTest}
class FrameBorder final {
private:
    /// Identifies the line style used at a border joint.
    enum class JointStyle : std::uint8_t {
        None,
        Light,
        Heavy,
        Double,
        LightRounded,
    };

public:
    /// The named line groups for this border.
    using Element = FrameBorderElement;

    /// Style and color for one border element.
    struct Border final {
        FrameStyle style{FrameStyle::None}; ///< The frame style for this element.
        Color color{};                      ///< The color for this element.

        auto operator==(const Border &other) const noexcept -> bool = default;
    };

public:
    /// Create a border with all elements set to `FrameStyle::None`.
    FrameBorder() = default;
    /// Create a border with the same style and color for all elements.
    /// @param style The style to apply to all elements.
    /// @param color The color to apply to all elements.
    explicit FrameBorder(FrameStyle style, Color color = {}) noexcept;

    // defaults
    ~FrameBorder() = default;
    FrameBorder(const FrameBorder &) = default;
    FrameBorder(FrameBorder &&) noexcept = default;
    auto operator=(const FrameBorder &) -> FrameBorder & = default;
    auto operator=(FrameBorder &&) noexcept -> FrameBorder & = default;

public: // operators
    auto operator==(const FrameBorder &other) const noexcept -> bool = default;

public: // accessors
    /// Access the style and color for an element.
    /// @param element The element to query.
    /// @return The configured style and color.
    [[nodiscard]] auto border(Element element) const noexcept -> const Border &;
    /// Access the frame style for an element.
    /// @param element The element to query.
    /// @return The configured frame style.
    [[nodiscard]] auto style(Element element) const noexcept -> FrameStyle;
    /// Access the color for an element.
    /// @param element The element to query.
    /// @return The configured color.
    [[nodiscard]] auto color(Element element) const noexcept -> Color;
    /// Set the style and color for an element.
    /// @param element The element to change.
    /// @param style The new frame style.
    /// @param color The new color.
    void set(Element element, FrameStyle style, Color color = {}) noexcept;

public:
    /// Test if a frame style can be used as a one-cell grid line style.
    /// @param style The style to test.
    /// @return `true` for `None` and supported line frame styles.
    [[nodiscard]] static auto isLineStyle(FrameStyle style) noexcept -> bool;
    /// Resolve the character for a grid corner or joint from its four directed borders.
    /// Color precedence is east, west, south, north; inherited color parts fall through to lower-priority borders.
    /// @param east The border segment going to the right.
    /// @param south The border segment going down.
    /// @param west The border segment going to the left.
    /// @param north The border segment going up.
    /// @return The resolved joint character.
    [[nodiscard]] static auto cornerChar(Border east, Border south, Border west, Border north) noexcept -> Block;
    /// Resolve one of the nine grid corner or joint characters for this border.
    /// @param anchor One of the nine composite anchor values.
    /// @return The resolved joint character.
    [[nodiscard]] auto corner(bgeo::BlockAnchor anchor) const noexcept -> Block;

private:
    /// Convert a border element to its storage index.
    [[nodiscard]] static constexpr auto indexForElement(Element element) noexcept -> std::size_t {
        return static_cast<std::size_t>(element);
    }
    /// Convert a frame style to its joint style.
    [[nodiscard]] static auto jointStyle(FrameStyle style) noexcept -> JointStyle;
    /// Convert a frame style to its joint-table style index.
    [[nodiscard]] static auto jointStyleIndex(FrameStyle style) noexcept -> std::size_t;
    /// Combine four directed joint-style indices into a table index.
    [[nodiscard]] static auto jointTableIndex(
        std::size_t east, std::size_t south, std::size_t west, std::size_t north) noexcept -> std::size_t;
    /// Resolve the Unicode code point for four directed frame styles.
    [[nodiscard]] static auto jointCodePoint(
        FrameStyle east, FrameStyle south, FrameStyle west, FrameStyle north) noexcept -> text::Char;
    /// Apply a border's color over an existing joint color.
    [[nodiscard]] static auto overlayBorderColor(Color currentColor, const Border &border) noexcept -> Color;
    /// Resolve the color of a joint from four directed borders.
    [[nodiscard]] static auto jointColor(Border east, Border south, Border west, Border north) noexcept -> Color;

private:
    std::array<Border, 6> _borders{};
};

}
