// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ReadLineApp.hpp"

#include <algorithm>
#include <array>

namespace demo {

using namespace el::text::literals;

void ReadLineApp::registerCommandLineOptions(const el::OptionsPtr &options) {
    options->setHelpTitle("ReadLine Demo"_el);
    options->setHelpDescription(
        "Interactively tests the non-blocking cterm ReadLine API while a live clock continues to update."_el);
    options->setHelpEpilog(
        "Boolean flags accept true/on/yes/enabled and false/off/no/disabled, case-insensitively. "
        "Styles use FG[:BG[:ATTRIBUTE,...]], for example bright_white:blue:bold,-italic. Frame elements use "
        "STYLE[;COLOR].\n\nAn ELCL file uses [ReadLine], [ReadLine.Styles], [ReadLine.Frame], and "
        "[ReadLine.Frame.Top] (also Bottom, Left, Right, H_Line, V_Line). ELCL values use native booleans, "
        "integers, time deltas such as 5 s or 250 ms, quoted text, and text lists. Command-line values override "
        "configuration values. Use --dump-config to print every resolved setting.\n\nExample:\n"
        "[ReadLine]\n"
        "prompt: \"Input: \"\n"
        "timeout: 10 s\n"
        "timeout display threshold: 5 s\n"
        "cleanup enabled: false\n"
        "[ReadLine.Styles]\n"
        "text: \"bright_white:bold\""_el);

    auto configurationOptions = el::OptionSet::create();
    configurationOptions->setHelpTitle("Configuration"_el);
    configurationOptions->addOption({"-c"_el, "--config"_el, "config"_el})
        .setType(el::OptionType::Text)
        .setValueName("file"_el)
        .setHelpDescription("Load ReadLine settings from an ELCL configuration file."_el);
    configurationOptions->addOption({"--dump-config"_el, "dump-config"_el})
        .setHelpDescription("Print the complete resolved ELCL configuration and exit."_el);

    auto contentOptions = el::OptionSet::create();
    contentOptions->setHelpTitle("Content and Editing"_el);
    contentOptions->addOption({"--title"_el, "title"_el})
        .setType(el::OptionType::Text)
        .setHelpDescription("Text displayed as the input title."_el);
    contentOptions->addOption({"--prompt"_el, "prompt"_el})
        .setType(el::OptionType::Text)
        .setHelpDescription("Text displayed before the editable input."_el);
    contentOptions->addOption({"--placeholder"_el, "placeholder"_el})
        .setType(el::OptionType::Text)
        .setHelpDescription("Text displayed while the input is empty."_el);
    contentOptions->addOption({"--current-text"_el, "current-text"_el})
        .setType(el::OptionType::Text)
        .setHelpDescription("Initial editable text."_el);
    contentOptions->addOption({"--history"_el, "history"_el})
        .setType(el::OptionType::Text)
        .setMaximum(el::ArgumentCount::infinite())
        .setHelpDescription("History entry; repeat this option to add multiple entries."_el);
    contentOptions->addOption({"--maximum-length"_el, "maximum-length"_el})
        .setType(el::OptionType::Integer)
        .setHelpDescription("Maximum number of entered Unicode code points."_el);
    contentOptions->addOption({"--maximum-lines"_el, "maximum-lines"_el})
        .setType(el::OptionType::Integer)
        .setHelpDescription("Maximum number of logical input lines."_el);
    contentOptions->addOption({"--maximum-display-lines"_el, "maximum-display-lines"_el})
        .setType(el::OptionType::Integer)
        .setHelpDescription("Maximum number of visible editor rows."_el);
    contentOptions->addOption({"--commit-key"_el, "commit-key"_el})
        .setType(el::OptionType::Text)
        .setValueName("key"_el)
        .setHelpDescription("Key binding that commits the input."_el);
    contentOptions->addOption({"--new-line-key"_el, "new-line-key"_el})
        .setType(el::OptionType::Text)
        .setValueName("key"_el)
        .setHelpDescription("Key binding that inserts a logical line break."_el);
    contentOptions->addOption({"--cancel-key"_el, "cancel-key"_el})
        .setType(el::OptionType::Text)
        .setValueName("key"_el)
        .setHelpDescription("Key binding that cancels the input."_el);

    auto timingOptions = el::OptionSet::create();
    timingOptions->setHelpTitle("Timing and Lifecycle"_el);
    timingOptions->addOption({"--timeout"_el, "timeout"_el})
        .setType(el::OptionType::Integer)
        .setValueName("seconds"_el)
        .setHelpDescription("Inactivity timeout in seconds; zero disables it."_el);
    timingOptions->addOption({"--timeout-display-threshold"_el, "timeout-display-threshold"_el})
        .setType(el::OptionType::Integer)
        .setValueName("seconds"_el)
        .setHelpDescription("Remaining seconds at which the countdown appears; zero hides it."_el);
    timingOptions->addOption({"--blink-interval"_el, "blink-interval"_el})
        .setType(el::OptionType::Integer)
        .setValueName("milliseconds"_el)
        .setHelpDescription("Positive cursor blink interval in milliseconds."_el);
    timingOptions->addOption({"--cleanup"_el, "cleanup"_el})
        .setType(el::OptionType::Flag)
        .setHelpDescription("Remove or retain the rendered input area when reading stops."_el);

    auto appearanceOptions = el::OptionSet::create();
    appearanceOptions->setHelpTitle("Appearance"_el);
    appearanceOptions->addOption({"--display-style"_el, "display-style"_el})
        .setChoices(el::OptionChoices::create({"compact"_el, "horizontal-space"_el, "horizontal-frame"_el, "frame"_el}))
        .setHelpDescription("Input-area layout."_el);
    appearanceOptions->addOption({"--padding-left"_el, "padding-left"_el})
        .setType(el::OptionType::Integer)
        .setHelpDescription("Left input padding in terminal cells."_el);
    appearanceOptions->addOption({"--padding-right"_el, "padding-right"_el})
        .setType(el::OptionType::Integer)
        .setHelpDescription("Right input padding in terminal cells."_el);
    appearanceOptions->addOption({"--cursor-block"_el, "cursor-block"_el})
        .setType(el::OptionType::Text)
        .setValueName("character"_el)
        .setHelpDescription("One-cell block displayed as the cursor over empty space."_el);
    constexpr auto cStyleNames = std::array{
        "background-style"_el,
        "title-style"_el,
        "prompt-style"_el,
        "placeholder-style"_el,
        "text-style"_el,
        "cursor-style"_el,
    };
    for (const auto &name : cStyleNames) {
        appearanceOptions->addOption({el::String::fromJoined({"--"_el, name}), name})
            .setType(el::OptionType::Text)
            .setValueName("style"_el)
            .setHelpDescription("Block style in FG[:BG[:ATTRIBUTE,...]] form."_el);
    }

    auto frameOptions = el::OptionSet::create();
    frameOptions->setHelpTitle("Frame"_el);
    frameOptions->addOption({"--frame-style"_el, "frame-style"_el})
        .setChoices(
            el::OptionChoices::create(
                {"none"_el,
                    "light"_el,
                    "light-double-dash"_el,
                    "light-triple-dash"_el,
                    "light-quadruple-dash"_el,
                    "heavy"_el,
                    "heavy-double-dash"_el,
                    "heavy-triple-dash"_el,
                    "heavy-quadruple-dash"_el,
                    "double"_el,
                    "rounded"_el}))
        .setHelpDescription("Uniform frame line style."_el);
    frameOptions->addOption({"--frame-color"_el, "frame-color"_el})
        .setType(el::OptionType::Text)
        .setValueName("color"_el)
        .setHelpDescription("Uniform frame color."_el);
    constexpr auto cFrameOptionNames = std::array{
        "frame-top"_el,
        "frame-bottom"_el,
        "frame-left"_el,
        "frame-right"_el,
        "frame-h-line"_el,
        "frame-v-line"_el,
    };
    for (const auto &name : cFrameOptionNames) {
        frameOptions->addOption({el::String::fromJoined({"--"_el, name}), name})
            .setType(el::OptionType::Text)
            .setValueName("style;color"_el)
            .setHelpDescription("Override one frame element using STYLE[;COLOR]."_el);
    }

    options->addSet(configurationOptions);
    options->addSet(contentOptions);
    options->addSet(timingOptions);
    options->addSet(appearanceOptions);
    options->addSet(frameOptions);
}

auto ReadLineApp::hasCommandLineValue(const el::String &name) const -> bool {
    const auto value = optionValues()->value(name);
    return value != nullptr && !value->argumentIndexes().empty();
}

void ReadLineApp::applyCommandLine(el::cterm::ReadLineOptions &settings) {
    const auto values = optionValues();
    if (hasCommandLineValue("title"_el)) {
        settings.setTitle(values->getText("title"_el));
    }
    if (hasCommandLineValue("prompt"_el)) {
        settings.setPrompt(values->getText("prompt"_el));
    }
    if (hasCommandLineValue("placeholder"_el)) {
        settings.setPlaceholder(values->getText("placeholder"_el));
    }
    if (hasCommandLineValue("current-text"_el)) {
        settings.setCurrentText(values->getText("current-text"_el));
    }
    if (hasCommandLineValue("history"_el)) {
        settings.setHistory(el::StringList{values->getTextList("history"_el)});
    }
    if (hasCommandLineValue("maximum-length"_el)) {
        const auto value = values->getInteger("maximum-length"_el);
        if (value < 0) {
            throw el::ApplicationError{"Maximum length must not be negative."_el};
        }
        settings.setMaximumLength(el::CpLength::fromSizeT(static_cast<std::size_t>(value)));
    }
    if (hasCommandLineValue("maximum-lines"_el)) {
        const auto value = values->getInteger("maximum-lines"_el);
        if (value <= 0) {
            throw el::ApplicationError{"Maximum lines must be greater than zero."_el};
        }
        settings.setMaximumLines(el::LineCount::fromSizeT(static_cast<std::size_t>(value)));
    }
    if (hasCommandLineValue("maximum-display-lines"_el)) {
        const auto value = values->getInteger("maximum-display-lines"_el);
        if (value <= 0) {
            throw el::ApplicationError{"Maximum display lines must be greater than zero."_el};
        }
        settings.setMaximumDisplayLines(el::LineCount::fromSizeT(static_cast<std::size_t>(value)));
    }
    if (hasCommandLineValue("timeout"_el)) {
        settings.setTimeout(el::Seconds{values->getInteger("timeout"_el)});
    }
    if (hasCommandLineValue("timeout-display-threshold"_el)) {
        settings.setTimeoutDisplayThreshold(el::Seconds{values->getInteger("timeout-display-threshold"_el)});
    }
    if (hasCommandLineValue("blink-interval"_el)) {
        settings.setBlinkInterval(el::Milliseconds{values->getInteger("blink-interval"_el)});
    }
    if (hasCommandLineValue("cleanup"_el)) {
        settings.setCleanupEnabled(values->getFlag("cleanup"_el));
    }
    if (hasCommandLineValue("display-style"_el)) {
        settings.setDisplayStyle(parseDisplayStyle(values->getText("display-style"_el)));
    }
    auto padding = settings.padding();
    if (hasCommandLineValue("padding-left"_el)) {
        padding.setLeading(el::block::Coordinate{values->getInteger("padding-left"_el)});
    }
    if (hasCommandLineValue("padding-right"_el)) {
        padding.setTrailing(el::block::Coordinate{values->getInteger("padding-right"_el)});
    }
    settings.setPadding(padding);
    if (hasCommandLineValue("cursor-block"_el)) {
        settings.setCursorBlock(parseBlock(values->getText("cursor-block"_el), settings.cursorStyle()));
    }
    if (hasCommandLineValue("commit-key"_el)) {
        settings.setCommitKey(parseKey(values->getText("commit-key"_el)));
    }
    if (hasCommandLineValue("new-line-key"_el)) {
        settings.setNewLineKey(parseKey(values->getText("new-line-key"_el)));
    }
    if (hasCommandLineValue("cancel-key"_el)) {
        settings.setCancelKey(parseKey(values->getText("cancel-key"_el)));
    }

    if (hasCommandLineValue("background-style"_el)) {
        settings.setBackgroundStyle(el::cterm::BlockStyle::fromStringOrThrow(values->getText("background-style"_el)));
    }
    if (hasCommandLineValue("title-style"_el)) {
        settings.setTitleStyle(el::cterm::BlockStyle::fromStringOrThrow(values->getText("title-style"_el)));
    }
    if (hasCommandLineValue("prompt-style"_el)) {
        settings.setPromptStyle(el::cterm::BlockStyle::fromStringOrThrow(values->getText("prompt-style"_el)));
    }
    if (hasCommandLineValue("placeholder-style"_el)) {
        settings.setPlaceholderStyle(el::cterm::BlockStyle::fromStringOrThrow(values->getText("placeholder-style"_el)));
    }
    if (hasCommandLineValue("text-style"_el)) {
        settings.setTextStyle(el::cterm::BlockStyle::fromStringOrThrow(values->getText("text-style"_el)));
    }
    if (hasCommandLineValue("cursor-style"_el)) {
        settings.setCursorStyle(el::cterm::BlockStyle::fromStringOrThrow(values->getText("cursor-style"_el)));
    }

    auto border = settings.frameBorder();
    if (hasCommandLineValue("frame-style"_el)) {
        applyUniformFrameStyle(border, parseFrameStyle(values->getText("frame-style"_el)));
    }
    if (hasCommandLineValue("frame-color"_el)) {
        applyUniformFrameColor(border, el::cterm::Color::fromStringOrThrow(values->getText("frame-color"_el)));
    }
    constexpr auto cFrameOptionNames = std::array{
        "frame-top"_el,
        "frame-bottom"_el,
        "frame-left"_el,
        "frame-right"_el,
        "frame-h-line"_el,
        "frame-v-line"_el,
    };
    constexpr auto cFrameElements = std::array{
        el::cterm::FrameBorder::Element::Top,
        el::cterm::FrameBorder::Element::Bottom,
        el::cterm::FrameBorder::Element::Left,
        el::cterm::FrameBorder::Element::Right,
        el::cterm::FrameBorder::Element::HLine,
        el::cterm::FrameBorder::Element::VLine,
    };
    for (auto index = std::size_t{0}; index < cFrameOptionNames.size(); ++index) {
        if (hasCommandLineValue(cFrameOptionNames[index])) {
            applyFrameStyle(border, cFrameElements[index], values->getText(cFrameOptionNames[index]));
        }
    }
    settings.setFrameBorder(std::move(border));
}

auto ReadLineApp::configurationValue(const el::conf::DocumentPtr &document, const el::String &path)
    -> el::conf::ValuePtr {
    return document == nullptr ? el::conf::ValuePtr{} : document->value(path);
}

void ReadLineApp::validateConfigurationKeys(const el::conf::DocumentPtr &document) const {
    constexpr auto cScalarConfigurationPaths = std::array{
        "readline.title"_el,
        "readline.prompt"_el,
        "readline.placeholder"_el,
        "readline.maximum_length"_el,
        "readline.maximum_lines"_el,
        "readline.maximum_display_lines"_el,
        "readline.history"_el,
        "readline.current_text"_el,
        "readline.timeout"_el,
        "readline.timeout_display_threshold"_el,
        "readline.blink_interval"_el,
        "readline.cursor_block"_el,
        "readline.commit_key"_el,
        "readline.new_line_key"_el,
        "readline.cancel_key"_el,
        "readline.cleanup_enabled"_el,
        "readline.display_style"_el,
        "readline.padding_left"_el,
        "readline.padding_right"_el,
        "readline.styles.background"_el,
        "readline.styles.title"_el,
        "readline.styles.prompt"_el,
        "readline.styles.placeholder"_el,
        "readline.styles.text"_el,
        "readline.styles.cursor"_el,
        "readline.frame.style"_el,
        "readline.frame.color"_el,
        "readline.frame.top.style"_el,
        "readline.frame.top.color"_el,
        "readline.frame.bottom.style"_el,
        "readline.frame.bottom.color"_el,
        "readline.frame.left.style"_el,
        "readline.frame.left.color"_el,
        "readline.frame.right.style"_el,
        "readline.frame.right.color"_el,
        "readline.frame.h_line.style"_el,
        "readline.frame.h_line.color"_el,
        "readline.frame.v_line.style"_el,
        "readline.frame.v_line.color"_el,
    };
    constexpr auto cConfigurationSectionPaths = std::array{
        "readline"_el,
        "readline.styles"_el,
        "readline.frame"_el,
        "readline.frame.top"_el,
        "readline.frame.bottom"_el,
        "readline.frame.left"_el,
        "readline.frame.right"_el,
        "readline.frame.h_line"_el,
        "readline.frame.v_line"_el,
    };
    for (const auto &[namePath, value] : document->toFlatValueMap()) {
        const auto path = namePath.toText();
        const auto isSection = std::ranges::any_of(
            cConfigurationSectionPaths, [&path](const auto &allowed) -> bool { return path == allowed; });
        const auto isScalar = std::ranges::any_of(
            cScalarConfigurationPaths, [&path](const auto &allowed) -> bool { return path == allowed; });
        if (!isSection && !isScalar) {
            throwConfigurationError(
                "Unknown ReadLine configuration key"_el,
                el::StringFormat{"\"{}\" is not supported by this demo."_el}.build(path),
                std::const_pointer_cast<el::conf::Value>(value));
        }
    }
}

[[noreturn]] void ReadLineApp::throwConfigurationError(
    el::String title, el::String description, const el::conf::ValuePtr &value, const std::exception_ptr &cause) {
    auto context = el::core::ApplicationErrorContext{std::move(title), std::move(description)};
    if (value != nullptr && value->hasLocation()) {
        const auto location = value->location();
        context.setCodeLocation(location.codeLocation());
        if (location.sourceIdentifier() != nullptr) {
            context.setSourceName(location.sourceIdentifier()->name());
            context.setSourcePath(location.sourceIdentifier()->path());
        }
    }
    throw el::ApplicationError{std::move(context), cause};
}

}
