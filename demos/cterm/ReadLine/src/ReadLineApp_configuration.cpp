// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ReadLineApp.hpp"

#include <array>
#include <utility>

namespace demo {

using namespace el::text::literals;

void ReadLineApp::applyConfiguration(el::cterm::ReadLineOptions &settings) {
    if (!hasCommandLineValue("config"_el)) {
        return;
    }
    const auto path = el::Path{optionValues()->getText("config"_el)};
    if (path.isEmpty()) {
        throw el::ApplicationError{"The configuration path is invalid."_el};
    }
    auto parser = el::conf::Parser{};
    const auto document = parser.parseFileOrThrow(path);
    validateConfigurationKeys(document);

    const auto applyText = [&](const el::String &path, const auto &setter) -> void {
        el::conf::ValuePtr value;
        try {
            value = document->value(path);
            if (value != nullptr) {
                setter(value->asTextOrThrow());
            }
        } catch (const el::Exception &error) {
            throwConfigurationError(
                "Failed to Read the Configuration"_el,
                "A value in the configuration has an unexpected format."_el,
                value,
                std::make_exception_ptr(error));
        }
    };
    applyText("readline.title"_el, [&](const el::String &value) -> void { settings.setTitle(value); });
    applyText("readline.prompt"_el, [&](const el::String &value) -> void { settings.setPrompt(value); });
    applyText("readline.placeholder"_el, [&](const el::String &value) -> void { settings.setPlaceholder(value); });
    applyText("readline.current_text"_el, [&](el::String value) -> void { settings.setCurrentText(std::move(value)); });
    if (document->hasValue("readline.history"_el)) {
        settings.setHistory(el::StringList{document->getListOrThrow<el::String>("readline.history"_el)});
    }

    const auto applyNonNegative = [&](const el::String &path, const auto &setter) -> void {
        if (!document->hasValue(path)) {
            return;
        }
        const auto value = document->getIntegerOrThrow(path);
        if (value < 0) {
            throwConfigurationError(
                "Invalid negative configuration value"_el,
                el::StringFormat{"\"{}\" must not be negative."_el}.build(path),
                configurationValue(document, path));
        }
        setter(static_cast<std::size_t>(value));
    };
    applyNonNegative("readline.maximum_length"_el, [&](const std::size_t value) -> void {
        settings.setMaximumLength(el::CpLength::fromSizeT(value));
    });
    applyNonNegative("readline.maximum_lines"_el, [&](const std::size_t value) -> void {
        if (value == 0U) {
            throwConfigurationError(
                "Invalid zero configuration value"_el,
                "ReadLine.maximum_lines must be greater than zero."_el,
                configurationValue(document, "readline.maximum_lines"_el));
        }
        settings.setMaximumLines(el::LineCount::fromSizeT(value));
    });
    applyNonNegative("readline.maximum_display_lines"_el, [&](const std::size_t value) -> void {
        if (value == 0U) {
            throwConfigurationError(
                "Invalid zero configuration value"_el,
                "ReadLine.maximum_display_lines must be greater than zero."_el,
                configurationValue(document, "readline.maximum_display_lines"_el));
        }
        settings.setMaximumDisplayLines(el::LineCount::fromSizeT(value));
    });
    applyNonNegative("readline.padding_left"_el, [&](const std::size_t value) -> void {
        auto padding = settings.padding();
        padding.setLeft(el::BlockCoordinate{value});
        settings.setPadding(padding);
    });
    applyNonNegative("readline.padding_right"_el, [&](const std::size_t value) -> void {
        auto padding = settings.padding();
        padding.setRight(el::BlockCoordinate{value});
        settings.setPadding(padding);
    });

    if (document->hasValue("readline.timeout"_el)) {
        settings.setTimeout(document->getCalendarDeltaOrThrow("readline.timeout"_el).toTimeDeltaOrThrow().toSeconds());
    }
    if (document->hasValue("readline.timeout_display_threshold"_el)) {
        settings.setTimeoutDisplayThreshold(document->getCalendarDeltaOrThrow("readline.timeout_display_threshold"_el)
                .toTimeDeltaOrThrow()
                .toSeconds());
    }
    if (document->hasValue("readline.blink_interval"_el)) {
        settings.setBlinkInterval(
            document->getCalendarDeltaOrThrow("readline.blink_interval"_el).toTimeDeltaOrThrow().toMilliseconds());
    }
    if (document->hasValue("readline.cleanup_enabled"_el)) {
        settings.setCleanupEnabled(document->getBooleanOrThrow("readline.cleanup_enabled"_el));
    }

    applyText("readline.display_style"_el, [&](const el::String &value) -> void {
        settings.setDisplayStyle(parseDisplayStyle(value));
    });
    applyText("readline.cursor_block"_el, [&](const el::String &value) -> void {
        settings.setCursorBlock(parseBlock(value, settings.cursorStyle()));
    });
    applyText(
        "readline.commit_key"_el, [&](const el::String &value) -> void { settings.setCommitKey(parseKey(value)); });
    applyText(
        "readline.new_line_key"_el, [&](const el::String &value) -> void { settings.setNewLineKey(parseKey(value)); });
    applyText(
        "readline.cancel_key"_el, [&](const el::String &value) -> void { settings.setCancelKey(parseKey(value)); });

    applyText("readline.styles.background"_el, [&](const el::String &value) -> void {
        settings.setBackgroundStyle(el::cterm::BlockStyle::fromStringOrThrow(value));
    });
    applyText("readline.styles.title"_el, [&](const el::String &value) -> void {
        settings.setTitleStyle(el::cterm::BlockStyle::fromStringOrThrow(value));
    });
    applyText("readline.styles.prompt"_el, [&](const el::String &value) -> void {
        settings.setPromptStyle(el::cterm::BlockStyle::fromStringOrThrow(value));
    });
    applyText("readline.styles.placeholder"_el, [&](const el::String &value) -> void {
        settings.setPlaceholderStyle(el::cterm::BlockStyle::fromStringOrThrow(value));
    });
    applyText("readline.styles.text"_el, [&](const el::String &value) -> void {
        settings.setTextStyle(el::cterm::BlockStyle::fromStringOrThrow(value));
    });
    applyText("readline.styles.cursor"_el, [&](const el::String &value) -> void {
        settings.setCursorStyle(el::cterm::BlockStyle::fromStringOrThrow(value));
    });

    auto border = settings.frameBorder();
    applyText("readline.frame.style"_el, [&](const el::String &value) -> void {
        applyUniformFrameStyle(border, parseFrameStyle(value));
    });
    applyText("readline.frame.color"_el, [&](const el::String &value) -> void {
        applyUniformFrameColor(border, el::cterm::Color::fromStringOrThrow(value));
    });
    constexpr auto elements = std::array{
        el::cterm::FrameBorder::Element::Top,
        el::cterm::FrameBorder::Element::Bottom,
        el::cterm::FrameBorder::Element::Left,
        el::cterm::FrameBorder::Element::Right,
        el::cterm::FrameBorder::Element::HLine,
        el::cterm::FrameBorder::Element::VLine,
    };
    constexpr auto paths = std::array{
        "readline.frame.top"_el,
        "readline.frame.bottom"_el,
        "readline.frame.left"_el,
        "readline.frame.right"_el,
        "readline.frame.h_line"_el,
        "readline.frame.v_line"_el,
    };
    for (auto index = std::size_t{0}; index < elements.size(); ++index) {
        const auto stylePath = el::String::fromJoined({paths[index], ".style"_el});
        const auto colorPath = el::String::fromJoined({paths[index], ".color"_el});
        if (document->hasValue(stylePath)) {
            border.set(
                elements[index], parseFrameStyle(document->getTextOrThrow(stylePath)), border.color(elements[index]));
        }
        if (document->hasValue(colorPath)) {
            border.set(
                elements[index],
                border.style(elements[index]),
                el::cterm::Color::fromStringOrThrow(document->getTextOrThrow(colorPath)));
        }
    }
    settings.setFrameBorder(std::move(border));
}

}
