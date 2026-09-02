// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "LogViewerApp.hpp"

#include <algorithm>
#include <array>
#include <utility>

namespace demo {

void LogViewerApp::beforeInitialize() {
    _updateSettings.setMinimumSize(Size{Coordinate{58}, Coordinate{12}});
    _updateSettings.setMinimumSizeBackground(Block{U' ', bg::Black});
    _updateSettings.setMinimumSizeMessage(
        BlockString{
            "Resize the terminal to at least 58x12 cells for the log viewer."_el, Color{fg::BrightWhite, bg::Black}});
}

auto LogViewerApp::beforeMain() -> int {
    scheduleNextMessage();
    return 0;
}

auto LogViewerApp::canvasSize() noexcept -> Size {
    if (_buffer.size().isZero()) {
        return terminal()->size().expandedWith(_updateSettings.minimumSize());
    }
    return _buffer.size();
}

void LogViewerApp::onKey(const Key &key) {
    const auto viewSize = contentRectForBuffer(canvasSize()).size();
    if (key == U'q' || key == Key::Escape) {
        _quitRequested = true;
    } else if (key == U'+' || key == U'=') {
        adjustDelayPreset(-1);
    } else if (key == U'-') {
        adjustDelayPreset(1);
    } else if (key == U'f') {
        _followMode = true;
        _viewOffset = clampViewOffset(
            Position{Coordinate{0}, Coordinate{_logBuffer->size().height() - viewSize.height()}},
            viewSize,
            _logBuffer->size());
    } else if (key == Key::Left) {
        _followMode = false;
        _viewOffset =
            clampViewOffset(_viewOffset + Position{Coordinate{-1}, Coordinate{0}}, viewSize, _logBuffer->size());
    } else if (key == Key::Right) {
        _followMode = false;
        _viewOffset =
            clampViewOffset(_viewOffset + Position{Coordinate{1}, Coordinate{0}}, viewSize, _logBuffer->size());
    } else if (key == Key::Up) {
        _followMode = false;
        _viewOffset =
            clampViewOffset(_viewOffset + Position{Coordinate{0}, Coordinate{-1}}, viewSize, _logBuffer->size());
    } else if (key == Key::Down) {
        _followMode = false;
        _viewOffset =
            clampViewOffset(_viewOffset + Position{Coordinate{0}, Coordinate{1}}, viewSize, _logBuffer->size());
    }
}

void LogViewerApp::adjustDelayPreset(const int delta) noexcept {
    const auto presets = delayPresets();
    const auto maximumIndex = static_cast<int>(presets.size()) - 1;
    _delayPresetIndex =
        static_cast<std::size_t>(std::clamp(static_cast<int>(_delayPresetIndex) + delta, 0, maximumIndex));
    _nextMessageAt = std::chrono::steady_clock::now() + randomDelay();
}

void LogViewerApp::scheduleNextMessage() noexcept {
    if (_nextMessageAt == std::chrono::steady_clock::time_point{}) {
        _nextMessageAt = std::chrono::steady_clock::now() + randomDelay();
    } else {
        _nextMessageAt += randomDelay();
    }
}

void LogViewerApp::appendGeneratedMessage() {
    renderLogMessage(generateLogMessage());
    _messageCount += 1;
}

void LogViewerApp::onRenderToBuffer() {
    auto now = std::chrono::steady_clock::now();
    while (!_quitRequested && now >= _nextMessageAt) {
        appendGeneratedMessage();
        scheduleNextMessage();
        now = std::chrono::steady_clock::now();
    }
    _buffer.fill(Block{U' ', bg::Black});
    const auto outerRect = Rectangle{
        Coordinate{0}, Coordinate{0}, Coordinate{_buffer.size().width()}, Coordinate{_buffer.size().height()}};
    const auto titleRect =
        Rectangle{Coordinate{2}, Coordinate{1}, Coordinate{_buffer.size().width() - 4}, Coordinate{1}};
    const auto contentRect = contentRectForBuffer(_buffer.size());
    const auto footerRect = Rectangle{
        Coordinate{2}, Coordinate{_buffer.size().height() - 2}, Coordinate{_buffer.size().width() - 4}, Coordinate{1}};
    _buffer.drawFrame(outerRect, FrameStyle::LightWithRoundedCorners);
    drawHeader(titleRect);
    drawLogView(contentRect);
    drawFooter(footerRect);
}

void LogViewerApp::drawHeader(const Rectangle rect) {
    _buffer.drawBlockText(
        el::StringFormat{"Log Viewer  |  CursorBuffer {}x{} / 250x500  |  {} mode  |  delay {}"_el}.build(
            _logBuffer->size().width(),
            _logBuffer->size().height(),
            _followMode ? "follow"_el : "manual"_el,
            delayPresets()[_delayPresetIndex].label),
        rect,
        Alignment::CenterLeft,
        Color{fg::BrightWhite, bg::Black});
    _buffer.drawBlockText(
        el::StringFormat{"entries {:04d}"_el}.build(static_cast<int>(_messageCount)),
        rect,
        Alignment::CenterRight,
        Color{fg::BrightCyan, bg::Black});
}

void LogViewerApp::drawFooter(const Rectangle rect) {
    _buffer.fill(rect, Block{U' ', bg::BrightBlack});
    auto footer = BlockStringEditor{};
    footer.append(
        fg::BrightCyan,
        Key{Key::Left}.toDisplayText(),
        " "_el,
        Key{Key::Right}.toDisplayText(),
        " "_el,
        Key{Key::Up}.toDisplayText(),
        " "_el,
        Key{Key::Down}.toDisplayText(),
        fg::BrightWhite,
        " pan  "_el,
        fg::BrightYellow,
        "[+][-]"_el,
        fg::BrightWhite,
        " timing  "_el,
        fg::BrightYellow,
        "[F]"_el,
        fg::BrightWhite,
        " follow newest  "_el,
        fg::BrightYellow,
        "[Q]"_el,
        " "_el,
        Key{Key::Escape}.toDisplayText(),
        fg::BrightWhite,
        " quit  "_el,
        fg::BrightGreen,
        el::StringFormat{"view ({}, {})"_el}.build(_logView.viewRect().x1(), _logView.viewRect().y1()));
    _buffer.drawBlockText(BlockText{footer, rect, Alignment::CenterLeft});
}

void LogViewerApp::drawLogView(const Rectangle rect) {
    _buffer.fill(rect, Block{U' ', Color{fg::Default, bg::Black}});
    updateView(rect.size());
    _buffer.drawBuffer(_logView, rect);
}

void LogViewerApp::updateView(const Size viewSize) noexcept {
    if (_followMode) {
        _viewOffset = clampViewOffset(
            Position{Coordinate{0}, Coordinate{_logBuffer->size().height() - viewSize.height()}},
            viewSize,
            _logBuffer->size());
    } else {
        _viewOffset = clampViewOffset(_viewOffset, viewSize, _logBuffer->size());
    }
    _logView.setViewRect(Rectangle{_viewOffset, viewSize});
}

void LogViewerApp::renderLogMessage(const LogMessage &message) {
    static const auto lineBreak = el::CharSet{U'\n'};
    auto lines = el::StringList::fromSplit(message.text, lineBreak);
    auto index = el::ItemIndex{0};
    if (!lines.isEmpty()) {
        renderInitialLine(nextTimestamp(), message.level, lines.getRefOrThrow(index));
    }
    ++index;
    while (index.isWithin(lines.count())) {
        renderContinuationLine(lines.getRefOrThrow(index));
        ++index;
    }
}

auto LogViewerApp::shouldCopyCell(const Block &cell) noexcept -> bool {
    const auto color = cell.color();
    const auto hasDefaultColors = color.fg() == fg::Default && color.bg() == bg::Default;
    if (!hasDefaultColors) {
        return true;
    }
    return !(cell.isEmpty() || cell == U' ');
}

void LogViewerApp::renderInitialLine(const el::String &timestamp, const LogLevel level, const el::String &text) {
    _logBuffer->setColor(logLevelColor(level));
    auto line = BlockStringEditor{timestamp, Color{fg::BrightWhite, bg::Black}};
    line += BlockString{" "_el, Color{fg::White, bg::Black}};
    line += BlockString{logTypeCode(level), logLevelColor(level)};
    line += BlockString{" "_el, logLevelColor(level)};
    line += BlockString{text, logLevelColor(level)};
    _logBuffer->printParagraph(line, initialLineOptions());
}

void LogViewerApp::renderContinuationLine(const el::String &text) {
    _logBuffer->setColor(Color{fg::White, bg::Black});
    _logBuffer->printParagraph(BlockString{text, Color{fg::Inherited, bg::Inherited}}, continuationLineOptions());
}

}
