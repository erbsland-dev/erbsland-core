// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ReadLineApp.hpp"

#include <array>

namespace demo {

using namespace el::text::literals;

auto ReadLineApp::quoted(const el::String &value) -> el::String {
    return el::String::fromJoined(
        {"\""_el, value.toEscaped(el::EscapeFormat::Config, el::EscapeAmount::Required), "\""_el});
}

auto ReadLineApp::plainText(const el::cterm::BlockString &value) -> el::String {
    auto result = el::StringEditor{};
    for (const auto &block : value) {
        result.append(block.toString());
    }
    return el::String{result};
}

auto ReadLineApp::displayStyleText(const el::cterm::ReadLineDisplayStyle style) -> el::String {
    switch (style) {
    case el::cterm::ReadLineDisplayStyle::Compact:
        return "compact"_el;
    case el::cterm::ReadLineDisplayStyle::HorizontalSpace:
        return "horizontal-space"_el;
    case el::cterm::ReadLineDisplayStyle::HorizontalFrame:
        return "horizontal-frame"_el;
    case el::cterm::ReadLineDisplayStyle::Frame:
        return "frame"_el;
    }
    return "horizontal-frame"_el;
}

auto ReadLineApp::frameStyleText(const el::cterm::FrameStyle style) -> el::String {
    switch (style) {
    case el::cterm::FrameStyle::None:
        return "none"_el;
    case el::cterm::FrameStyle::Light:
        return "light"_el;
    case el::cterm::FrameStyle::LightDoubleDash:
        return "light-double-dash"_el;
    case el::cterm::FrameStyle::LightTripleDash:
        return "light-triple-dash"_el;
    case el::cterm::FrameStyle::LightQuadrupleDash:
        return "light-quadruple-dash"_el;
    case el::cterm::FrameStyle::Heavy:
        return "heavy"_el;
    case el::cterm::FrameStyle::HeavyDoubleDash:
        return "heavy-double-dash"_el;
    case el::cterm::FrameStyle::HeavyTripleDash:
        return "heavy-triple-dash"_el;
    case el::cterm::FrameStyle::HeavyQuadrupleDash:
        return "heavy-quadruple-dash"_el;
    case el::cterm::FrameStyle::Double:
        return "double"_el;
    case el::cterm::FrameStyle::LightWithRoundedCorners:
        return "rounded"_el;
    default:
        return "none"_el;
    }
}

void ReadLineApp::appendAssignment(el::StringEditor &result, const el::String &name, const el::String &value) {
    result.append(name);
    result.append(": "_el);
    result.append(value);
    result.append("\n"_el);
}

void ReadLineApp::appendTextAssignment(el::StringEditor &result, const el::String &name, const el::String &value) {
    appendAssignment(result, name, quoted(value));
}

void ReadLineApp::dumpConfiguration(const el::cterm::ReadLineOptions &settings) {
    auto result = el::StringEditor{"@version: \"1.0\"\n"
                                   "# Complete resolved configuration for the ReadLine demo.\n\n"
                                   "[readline]\n"_el};
    appendTextAssignment(result, "title"_el, plainText(settings.title()));
    appendTextAssignment(result, "prompt"_el, plainText(settings.prompt()));
    appendTextAssignment(result, "placeholder"_el, plainText(settings.placeholder()));
    appendTextAssignment(result, "current_text"_el, settings.currentText());
    if (settings.history().isEmpty()) {
        result.append("# history: \"previous entry\" # No history entries are currently configured.\n"_el);
    } else {
        result.append("history: "_el);
        auto first = true;
        for (const auto &entry : settings.history()) {
            if (!first) {
                result.append(", "_el);
            }
            first = false;
            result.append(quoted(entry));
        }
        result.append("\n"_el);
    }
    appendAssignment(result, "maximum_length"_el, el::String::fromInteger(settings.maximumLength().toRawValue()));
    appendAssignment(result, "maximum_lines"_el, el::String::fromInteger(settings.maximumLines().toRawValue()));
    appendAssignment(
        result, "maximum_display_lines"_el, el::String::fromInteger(settings.maximumDisplayLines().toRawValue()));
    appendAssignment(result, "timeout"_el, el::StringFormat{"{} s"_el}.build(settings.timeout().toRawValue()));
    appendAssignment(
        result,
        "timeout_display_threshold"_el,
        el::StringFormat{"{} s"_el}.build(settings.timeoutDisplayThreshold().toRawValue()));
    appendAssignment(
        result, "blink_interval"_el, el::StringFormat{"{} ms"_el}.build(settings.blinkInterval().toRawValue()));
    appendAssignment(result, "cleanup_enabled"_el, settings.cleanupEnabled() ? "true"_el : "false"_el);
    appendTextAssignment(result, "display_style"_el, displayStyleText(settings.displayStyle()));
    appendAssignment(result, "padding_left"_el, el::String::fromInteger(settings.padding().leading().toRawValue()));
    appendAssignment(result, "padding_right"_el, el::String::fromInteger(settings.padding().trailing().toRawValue()));
    appendTextAssignment(result, "cursor_block"_el, settings.cursorBlock().toString());
    appendTextAssignment(result, "commit_key"_el, settings.commitKey().toString());
    appendTextAssignment(result, "new_line_key"_el, settings.newLineKey().toString());
    appendTextAssignment(result, "cancel_key"_el, settings.cancelKey().toString());

    result.append("\n[readline.styles]\n"_el);
    appendTextAssignment(result, "background"_el, settings.backgroundStyle().toString());
    appendTextAssignment(result, "title"_el, settings.titleStyle().toString());
    appendTextAssignment(result, "prompt"_el, settings.promptStyle().toString());
    appendTextAssignment(result, "placeholder"_el, settings.placeholderStyle().toString());
    appendTextAssignment(result, "text"_el, settings.textStyle().toString());
    appendTextAssignment(result, "cursor"_el, settings.cursorStyle().toString());

    constexpr auto elements = std::array{
        el::cterm::FrameBorder::Element::Top,
        el::cterm::FrameBorder::Element::Bottom,
        el::cterm::FrameBorder::Element::Left,
        el::cterm::FrameBorder::Element::Right,
        el::cterm::FrameBorder::Element::HLine,
        el::cterm::FrameBorder::Element::VLine,
    };
    constexpr auto names = std::array{
        "top"_el,
        "bottom"_el,
        "left"_el,
        "right"_el,
        "h_line"_el,
        "v_line"_el,
    };
    for (auto index = std::size_t{0}; index < elements.size(); ++index) {
        result.append("\n[readline.frame."_el);
        result.append(names[index]);
        result.append("]\n"_el);
        appendTextAssignment(result, "style"_el, frameStyleText(settings.frameBorder().style(elements[index])));
        appendTextAssignment(result, "color"_el, settings.frameBorder().color(elements[index]).toString());
    }

    el::io::print(el::String{result});
}

}
