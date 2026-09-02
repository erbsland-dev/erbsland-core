// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FrameBorder.hpp"

#include "impl/FrameBorder_data.hpp"

namespace erbsland::cterm {

static_assert(impl::cFrameBorderJointStyleCount == 5);

FrameBorder::FrameBorder(const FrameStyle style, const Color color) noexcept {
    _borders.fill(Border{.style = style, .color = color});
}

auto FrameBorder::border(const Element element) const noexcept -> const Border & {
    return _borders[indexForElement(element)];
}

auto FrameBorder::style(const Element element) const noexcept -> FrameStyle {
    return border(element).style;
}

auto FrameBorder::color(const Element element) const noexcept -> Color {
    return border(element).color;
}

void FrameBorder::set(const Element element, const FrameStyle style, const Color color) noexcept {
    _borders[indexForElement(element)] = Border{.style = style, .color = color};
}

auto FrameBorder::isLineStyle(const FrameStyle style) noexcept -> bool {
    switch (style) {
    case FrameStyle::None:
    case FrameStyle::Light:
    case FrameStyle::LightDoubleDash:
    case FrameStyle::LightTripleDash:
    case FrameStyle::LightQuadrupleDash:
    case FrameStyle::Heavy:
    case FrameStyle::HeavyDoubleDash:
    case FrameStyle::HeavyTripleDash:
    case FrameStyle::HeavyQuadrupleDash:
    case FrameStyle::Double:
    case FrameStyle::LightWithRoundedCorners:
        return true;
    default:
        return false;
    }
}

auto FrameBorder::jointStyle(const FrameStyle style) noexcept -> JointStyle {
    switch (style) {
    case FrameStyle::Light:
    case FrameStyle::LightDoubleDash:
    case FrameStyle::LightTripleDash:
    case FrameStyle::LightQuadrupleDash:
        return JointStyle::Light;
    case FrameStyle::Heavy:
    case FrameStyle::HeavyDoubleDash:
    case FrameStyle::HeavyTripleDash:
    case FrameStyle::HeavyQuadrupleDash:
        return JointStyle::Heavy;
    case FrameStyle::Double:
        return JointStyle::Double;
    case FrameStyle::LightWithRoundedCorners:
        return JointStyle::LightRounded;
    default:
        return JointStyle::None;
    }
}

auto FrameBorder::jointStyleIndex(const FrameStyle style) noexcept -> std::size_t {
    return static_cast<std::size_t>(jointStyle(style));
}

auto FrameBorder::jointTableIndex(
    const std::size_t east, const std::size_t south, const std::size_t west, const std::size_t north) noexcept
    -> std::size_t {

    return (((east * impl::cFrameBorderJointStyleCount) + south) * impl::cFrameBorderJointStyleCount + west) *
        impl::cFrameBorderJointStyleCount +
        north;
}

auto FrameBorder::jointCodePoint(
    const FrameStyle east, const FrameStyle south, const FrameStyle west, const FrameStyle north) noexcept
    -> text::Char {

    const auto tableIndex =
        jointTableIndex(jointStyleIndex(east), jointStyleIndex(south), jointStyleIndex(west), jointStyleIndex(north));
    const auto characterIndex = static_cast<std::uint8_t>(impl::cFrameBorderJointCharacterIndexes[tableIndex]);
    return text::Char{impl::cFrameBorderJointCharacters[characterIndex]};
}

auto FrameBorder::overlayBorderColor(const Color currentColor, const Border &border) noexcept -> Color {
    if (!isLineStyle(border.style) || border.style == FrameStyle::None) {
        return currentColor;
    }
    return currentColor.overlayWith(border.color);
}

auto FrameBorder::jointColor(const Border east, const Border south, const Border west, const Border north) noexcept
    -> Color {

    auto result = Color{};
    result = overlayBorderColor(result, north);
    result = overlayBorderColor(result, south);
    result = overlayBorderColor(result, west);
    result = overlayBorderColor(result, east);
    return result;
}

auto FrameBorder::cornerChar(const Border east, const Border south, const Border west, const Border north) noexcept
    -> Block {

    return Block{
        jointCodePoint(east.style, south.style, west.style, north.style), jointColor(east, south, west, north)};
}

auto FrameBorder::corner(const geometry::Anchor anchor) const noexcept -> Block {
    static const auto cNone = Border{};

    if (anchor == geometry::Anchor::TopLeft) {
        return cornerChar(border(Element::Top), border(Element::Left), cNone, cNone);
    }
    if (anchor == geometry::Anchor::TopCenter) {
        return cornerChar(border(Element::Top), border(Element::VLine), border(Element::Top), cNone);
    }
    if (anchor == geometry::Anchor::TopRight) {
        return cornerChar(cNone, border(Element::Right), border(Element::Top), cNone);
    }
    if (anchor == geometry::Anchor::CenterLeft) {
        return cornerChar(border(Element::HLine), border(Element::Left), cNone, border(Element::Left));
    }
    if (anchor == geometry::Anchor::Center) {
        return cornerChar(
            border(Element::HLine), border(Element::VLine), border(Element::HLine), border(Element::VLine));
    }
    if (anchor == geometry::Anchor::CenterRight) {
        return cornerChar(cNone, border(Element::Right), border(Element::HLine), border(Element::Right));
    }
    if (anchor == geometry::Anchor::BottomLeft) {
        return cornerChar(border(Element::Bottom), cNone, cNone, border(Element::Left));
    }
    if (anchor == geometry::Anchor::BottomCenter) {
        return cornerChar(border(Element::Bottom), cNone, border(Element::Bottom), border(Element::VLine));
    }
    if (anchor == geometry::Anchor::BottomRight) {
        return cornerChar(cNone, cNone, border(Element::Bottom), border(Element::Right));
    }
    return cornerChar(cNone, cNone, cNone, cNone);
}

}
