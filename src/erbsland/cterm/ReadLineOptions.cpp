// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ReadLineOptions.hpp"

#include "../err/ParameterFailure.hpp"
#include "../text/Literals.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::cterm {

using namespace err;
using namespace text;
using namespace text::literals;
using namespace unit;

ReadLineOptions::ReadLineOptions() :
    _cursorStyle{BlockAttributes::Reverse},
    _displayStyle{ReadLineDisplayStyle::HorizontalFrame},
    _frameBorder{FrameStyle::Light},
    _padding{1},
    _prompt{BlockString{"› "_el}},
    _timeout{time::Seconds::zero()},
    _timeoutDisplayThreshold{20},
    _maximumLength{CpCount{4096U}},
    _maximumLines{LineCount{1U}},
    _maximumDisplayLines{LineCount{5U}},
    _blinkInterval{800},
    _cursorBlock{Block{U'█'}},
    _commitKey{Key::Enter},
    _newLineKey{Key::F2},
    _cancelKey{Key::Escape},
    _cleanupEnabled{true} {
}

auto ReadLineOptions::setBackgroundStyle(BlockStyle style) noexcept -> ReadLineOptions & {
    _backgroundStyle = style;
    return *this;
}

auto ReadLineOptions::setTitleStyle(BlockStyle style) noexcept -> ReadLineOptions & {
    _titleStyle = style;
    return *this;
}

auto ReadLineOptions::setPromptStyle(BlockStyle style) noexcept -> ReadLineOptions & {
    _promptStyle = style;
    return *this;
}

auto ReadLineOptions::setPlaceholderStyle(BlockStyle style) noexcept -> ReadLineOptions & {
    _placeholderStyle = style;
    return *this;
}

auto ReadLineOptions::setTextStyle(BlockStyle style) noexcept -> ReadLineOptions & {
    _textStyle = style;
    return *this;
}

auto ReadLineOptions::setCursorStyle(BlockStyle style) noexcept -> ReadLineOptions & {
    _cursorStyle = style;
    return *this;
}

auto ReadLineOptions::setDisplayStyle(const ReadLineDisplayStyle style) noexcept -> ReadLineOptions & {
    _displayStyle = style;
    return *this;
}

auto ReadLineOptions::setFrameBorder(FrameBorder border) noexcept -> ReadLineOptions & {
    _frameBorder = std::move(border);
    return *this;
}

auto ReadLineOptions::setPadding(const block::MarginPair padding) noexcept -> ReadLineOptions & {
    _padding = padding.expandedPositive();
    return *this;
}

auto ReadLineOptions::setTitle(BlockString title) noexcept -> ReadLineOptions & {
    _title = std::move(title);
    return *this;
}

auto ReadLineOptions::setTitle(const String &title) -> ReadLineOptions & {
    return setTitle(BlockString{title});
}

auto ReadLineOptions::setPrompt(BlockString prompt) noexcept -> ReadLineOptions & {
    _prompt = std::move(prompt);
    return *this;
}

auto ReadLineOptions::setPrompt(const String &prompt) -> ReadLineOptions & {
    return setPrompt(BlockString{prompt});
}

auto ReadLineOptions::setPlaceholder(BlockString placeholder) noexcept -> ReadLineOptions & {
    _placeholder = std::move(placeholder);
    return *this;
}

auto ReadLineOptions::setPlaceholder(const String &placeholder) -> ReadLineOptions & {
    return setPlaceholder(BlockString{placeholder});
}

auto ReadLineOptions::setMaximumLength(const CpCount maximumLength) noexcept -> ReadLineOptions & {
    _maximumLength = maximumLength;
    return *this;
}

auto ReadLineOptions::setMaximumLines(const LineCount maximumLines) -> ReadLineOptions & {
    if (maximumLines.isZero()) {
        throw ParameterFailure{"The read-line maximum line count must be at least one."_el, "maximumLines"_el};
    }
    _maximumLines = maximumLines;
    return *this;
}

auto ReadLineOptions::setMaximumDisplayLines(const LineCount maximumDisplayLines) -> ReadLineOptions & {
    if (maximumDisplayLines.isZero()) {
        throw ParameterFailure{
            "The read-line maximum display line count must be at least one.",
            "maximumDisplayLines",
        };
    }
    _maximumDisplayLines = maximumDisplayLines;
    return *this;
}

auto ReadLineOptions::setHistory(StringList history) noexcept -> ReadLineOptions & {
    _history = std::move(history);
    return *this;
}

auto ReadLineOptions::setCurrentText(String currentText) noexcept -> ReadLineOptions & {
    _currentText = std::move(currentText);
    return *this;
}

auto ReadLineOptions::setTimeout(const time::Seconds timeout) -> ReadLineOptions & {
    if (timeout.isNegative()) {
        throw ParameterFailure{"The read-line timeout must not be negative."_el, "timeout"_el};
    }
    _timeout = timeout;
    return *this;
}

auto ReadLineOptions::setTimeoutDisplayThreshold(const time::Seconds timeoutDisplayThreshold) -> ReadLineOptions & {
    if (timeoutDisplayThreshold.isNegative()) {
        throw ParameterFailure{
            "The read-line timeout display threshold must not be negative."_el, "timeoutDisplayThreshold"_el};
    }
    _timeoutDisplayThreshold = timeoutDisplayThreshold;
    return *this;
}

auto ReadLineOptions::setBlinkInterval(const time::Milliseconds blinkInterval) -> ReadLineOptions & {
    if (!blinkInterval.isPositive()) {
        throw ParameterFailure{"The read-line blink interval must be positive."_el, "blinkInterval"_el};
    }
    _blinkInterval = blinkInterval;
    return *this;
}

auto ReadLineOptions::setCursorBlock(Block cursorBlock) -> ReadLineOptions & {
    if (cursorBlock.displayWidth() != 1 || cursorBlock.isControl()) {
        throw ParameterFailure{
            "The read-line cursor block must occupy exactly one terminal cell.",
            "cursorBlock",
        };
    }
    _cursorBlock = std::move(cursorBlock);
    return *this;
}

auto ReadLineOptions::setCommitKey(Key commitKey) noexcept -> ReadLineOptions & {
    _commitKey = std::move(commitKey);
    return *this;
}

auto ReadLineOptions::setNewLineKey(Key newLineKey) noexcept -> ReadLineOptions & {
    _newLineKey = std::move(newLineKey);
    return *this;
}

auto ReadLineOptions::setCancelKey(Key cancelKey) noexcept -> ReadLineOptions & {
    _cancelKey = std::move(cancelKey);
    return *this;
}

auto ReadLineOptions::setCleanupEnabled(const bool cleanupEnabled) noexcept -> ReadLineOptions & {
    _cleanupEnabled = cleanupEnabled;
    return *this;
}

}
