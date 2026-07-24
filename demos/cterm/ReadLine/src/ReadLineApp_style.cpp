// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ReadLineApp.hpp"

#include <array>
#include <compare>

namespace demo {

using namespace el::text::literals;

auto ReadLineApp::equalsCI(const el::String &left, const el::String &right) noexcept -> bool {
    return left.compare(right, el::cCaseInsensitive.asciiComparisonFn()) == std::strong_ordering::equal;
}

auto ReadLineApp::parseDisplayStyle(const el::String &text) -> el::cterm::ReadLineDisplayStyle {
    if (equalsCI(text, "compact"_el)) {
        return el::cterm::ReadLineDisplayStyle::Compact;
    }
    if (equalsCI(text, "horizontal-space"_el)) {
        return el::cterm::ReadLineDisplayStyle::HorizontalSpace;
    }
    if (equalsCI(text, "horizontal-frame"_el)) {
        return el::cterm::ReadLineDisplayStyle::HorizontalFrame;
    }
    if (equalsCI(text, "frame"_el)) {
        return el::cterm::ReadLineDisplayStyle::Frame;
    }
    throw el::ApplicationError{el::StringFormat{"Unsupported ReadLine display style \"{}\"."_el}.build(text)};
}

auto ReadLineApp::parseFrameStyle(const el::String &text) -> el::cterm::FrameStyle {
    struct Definition final {
        el::String name;
        el::cterm::FrameStyle style;
    };
    const auto definitions = std::array{
        Definition{"none"_el, el::cterm::FrameStyle::None},
        Definition{"light"_el, el::cterm::FrameStyle::Light},
        Definition{"light-double-dash"_el, el::cterm::FrameStyle::LightDoubleDash},
        Definition{"light-triple-dash"_el, el::cterm::FrameStyle::LightTripleDash},
        Definition{"light-quadruple-dash"_el, el::cterm::FrameStyle::LightQuadrupleDash},
        Definition{"heavy"_el, el::cterm::FrameStyle::Heavy},
        Definition{"heavy-double-dash"_el, el::cterm::FrameStyle::HeavyDoubleDash},
        Definition{"heavy-triple-dash"_el, el::cterm::FrameStyle::HeavyTripleDash},
        Definition{"heavy-quadruple-dash"_el, el::cterm::FrameStyle::HeavyQuadrupleDash},
        Definition{"double"_el, el::cterm::FrameStyle::Double},
        Definition{"rounded"_el, el::cterm::FrameStyle::LightWithRoundedCorners},
    };
    const auto normalized = text.trimmed();
    for (const auto &definition : definitions) {
        if (equalsCI(normalized, definition.name)) {
            return definition.style;
        }
    }
    throw el::ApplicationError{el::StringFormat{"Unsupported frame style \"{}\"."_el}.build(text)};
}

auto ReadLineApp::parseBlock(const el::String &text, const el::cterm::BlockStyle style) -> el::cterm::Block {
    if (text.isEmpty()) {
        throw el::ApplicationError{"A block character must not be empty."_el};
    }
    const auto block = el::cterm::Block{text, style};
    if (block.displayWidth() != 1) {
        throw el::ApplicationError{"A block character must occupy exactly one terminal cell."_el};
    }
    return block;
}

auto ReadLineApp::parseKey(const el::String &text) -> el::cterm::Key {
    const auto key = el::cterm::Key::fromString(text);
    if (!key.valid()) {
        throw el::ApplicationError{el::StringFormat{"Unsupported key binding \"{}\"."_el}.build(text)};
    }
    return key;
}

void ReadLineApp::applyFrameStyle(
    el::cterm::FrameBorder &border, const el::cterm::FrameBorder::Element element, const el::String &text) {
    const auto separator = text.find(";"_el);
    const auto style = parseFrameStyle(text.slice({el::ByteIndex::zero(), separator}).trimmed());
    auto color = border.color(element);
    if (!separator.isNoIndex()) {
        const auto colorText = text.slice({separator.incremented(), el::ByteLength::infinite()}).trimmed();
        if (colorText.isEmpty() || colorText.contains(";"_el)) {
            throw el::ApplicationError{"Frame element syntax is STYLE[;COLOR]."_el};
        }
        color = el::cterm::Color::fromStringOrThrow(colorText);
    }
    border.set(element, style, color);
}

void ReadLineApp::applyUniformFrameStyle(el::cterm::FrameBorder &border, const el::cterm::FrameStyle style) {
    constexpr auto elements = std::array{
        el::cterm::FrameBorder::Element::Top,
        el::cterm::FrameBorder::Element::Bottom,
        el::cterm::FrameBorder::Element::Left,
        el::cterm::FrameBorder::Element::Right,
        el::cterm::FrameBorder::Element::HLine,
        el::cterm::FrameBorder::Element::VLine,
    };
    for (const auto element : elements) {
        border.set(element, style, border.color(element));
    }
}

void ReadLineApp::applyUniformFrameColor(el::cterm::FrameBorder &border, const el::cterm::Color color) {
    constexpr auto elements = std::array{
        el::cterm::FrameBorder::Element::Top,
        el::cterm::FrameBorder::Element::Bottom,
        el::cterm::FrameBorder::Element::Left,
        el::cterm::FrameBorder::Element::Right,
        el::cterm::FrameBorder::Element::HLine,
        el::cterm::FrameBorder::Element::VLine,
    };
    for (const auto element : elements) {
        border.set(element, border.style(element), color);
    }
}

}
