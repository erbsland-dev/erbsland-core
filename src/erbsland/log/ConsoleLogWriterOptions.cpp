// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ConsoleLogWriterOptions.hpp"

#include <utility>

namespace erbsland::log {

ConsoleLogWriterOptions::ConsoleLogWriterOptions() {
    _lineLevels[levelIndex(LogLevel::Trace)] = cterm::BlockStyle{cterm::fg::BrightBlack};
    _lineLevels[levelIndex(LogLevel::Information)] = cterm::BlockStyle{cterm::fg::BrightWhite};
    _lineLevels[levelIndex(LogLevel::Warning)] = cterm::BlockStyle{cterm::fg::BrightYellow};
    _lineLevels[levelIndex(LogLevel::Error)] = cterm::BlockStyle{cterm::fg::BrightRed};
    _partBase[partIndex(LogLinePart::Time)] = cterm::BlockStyle{cterm::fg::BrightCyan};
    _partBase[partIndex(LogLinePart::Name)] = cterm::BlockStyle{cterm::fg::BrightMagenta};
}

auto ConsoleLogWriterOptions::setParagraphOptions(cterm::ParagraphOptions value) noexcept -> ConsoleLogWriterOptions & {
    _paragraph = std::move(value);
    return *this;
}

auto ConsoleLogWriterOptions::setBaseLineStyle(const cterm::BlockStyle style) noexcept -> ConsoleLogWriterOptions & {
    _lineBase = style;
    return *this;
}

auto ConsoleLogWriterOptions::lineStyle(const LogLevel level) const noexcept -> cterm::BlockStyle {
    return _lineBase.withOverlay(_lineLevels[levelIndex(level)]);
}

auto ConsoleLogWriterOptions::setLineStyle(const LogLevel level, const cterm::BlockStyle style) noexcept
    -> ConsoleLogWriterOptions & {
    _lineLevels[levelIndex(level)] = style;
    return *this;
}

auto ConsoleLogWriterOptions::partStyle(const LogLinePart part, const LogLevel level) const noexcept
    -> cterm::BlockStyle {
    return lineStyle(level)
        .withOverlay(_partBase[partIndex(part)])
        .withOverlay(_partLevels[partIndex(part)][levelIndex(level)]);
}

auto ConsoleLogWriterOptions::setPartStyle(const LogLinePart part, const cterm::BlockStyle style) noexcept
    -> ConsoleLogWriterOptions & {
    _partBase[partIndex(part)] = style;
    return *this;
}

auto ConsoleLogWriterOptions::setPartStyle(
    const LogLinePart part, const LogLevel level, const cterm::BlockStyle style) noexcept -> ConsoleLogWriterOptions & {
    _partLevels[partIndex(part)][levelIndex(level)] = style;
    return *this;
}

auto ConsoleLogWriterOptions::levelIndex(const LogLevel level) noexcept -> std::size_t {
    switch (level.toRawValue()) {
    case LogLevel::Trace:
        return 0U;
    case LogLevel::Information:
        return 1U;
    case LogLevel::Warning:
        return 2U;
    case LogLevel::Error:
        return 3U;
    case LogLevel::All:
        break;
    }
    return 0U;
}

auto ConsoleLogWriterOptions::partIndex(const LogLinePart part) noexcept -> std::size_t {
    return static_cast<std::size_t>(part.toRawValue());
}

}
