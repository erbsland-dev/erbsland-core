// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "LogViewerApp.hpp"

#include <algorithm>
#include <array>
#include <utility>

namespace demo {

void LogViewerApp::beforeInitialize() {
    _updateSettings.setMinimumSize(BlockSize{BlockCoordinate{58}, BlockCoordinate{12}});
    _updateSettings.setMinimumSizeBackground(Block{U' ', bg::Black});
    _updateSettings.setMinimumSizeMessage(
        BlockString{
            "Resize the terminal to at least 58x12 cells for the log viewer."_el, Color{fg::BrightWhite, bg::Black}});
}

auto LogViewerApp::beforeMain() -> int {
    scheduleNextMessage();
    return 0;
}

auto LogViewerApp::canvasSize() noexcept -> BlockSize {
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
            BlockPosition{BlockCoordinate{0}, BlockCoordinate{_logBuffer->size().height() - viewSize.height()}},
            viewSize,
            _logBuffer->size());
    } else if (key == Key::Left) {
        _followMode = false;
        _viewOffset = clampViewOffset(
            _viewOffset + BlockPosition{BlockCoordinate{-1}, BlockCoordinate{0}}, viewSize, _logBuffer->size());
    } else if (key == Key::Right) {
        _followMode = false;
        _viewOffset = clampViewOffset(
            _viewOffset + BlockPosition{BlockCoordinate{1}, BlockCoordinate{0}}, viewSize, _logBuffer->size());
    } else if (key == Key::Up) {
        _followMode = false;
        _viewOffset = clampViewOffset(
            _viewOffset + BlockPosition{BlockCoordinate{0}, BlockCoordinate{-1}}, viewSize, _logBuffer->size());
    } else if (key == Key::Down) {
        _followMode = false;
        _viewOffset = clampViewOffset(
            _viewOffset + BlockPosition{BlockCoordinate{0}, BlockCoordinate{1}}, viewSize, _logBuffer->size());
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
    const auto outerRect = BlockRectangle{
        BlockCoordinate{0},
        BlockCoordinate{0},
        BlockCoordinate{_buffer.size().width()},
        BlockCoordinate{_buffer.size().height()}};
    const auto titleRect = BlockRectangle{
        BlockCoordinate{2}, BlockCoordinate{1}, BlockCoordinate{_buffer.size().width() - 4}, BlockCoordinate{1}};
    const auto contentRect = contentRectForBuffer(_buffer.size());
    const auto footerRect = BlockRectangle{
        BlockCoordinate{2},
        BlockCoordinate{_buffer.size().height() - 2},
        BlockCoordinate{_buffer.size().width() - 4},
        BlockCoordinate{1}};
    _buffer.drawFrame(outerRect, FrameStyle::LightWithRoundedCorners);
    drawHeader(titleRect);
    drawLogView(contentRect);
    drawFooter(footerRect);
}

void LogViewerApp::drawHeader(const BlockRectangle rect) {
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

void LogViewerApp::drawFooter(const BlockRectangle rect) {
    _buffer.fill(rect, Block{U' ', bg::BrightBlack});
    auto footer = BlockString{};
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

void LogViewerApp::drawLogView(const BlockRectangle rect) {
    _buffer.fill(rect, Block{U' ', Color{fg::Default, bg::Black}});
    updateView(rect.size());
    _buffer.drawBuffer(_logView, rect);
}

void LogViewerApp::updateView(const BlockSize viewSize) noexcept {
    if (_followMode) {
        _viewOffset = clampViewOffset(
            BlockPosition{BlockCoordinate{0}, BlockCoordinate{_logBuffer->size().height() - viewSize.height()}},
            viewSize,
            _logBuffer->size());
    } else {
        _viewOffset = clampViewOffset(_viewOffset, viewSize, _logBuffer->size());
    }
    _logView.setViewRect(BlockRectangle{_viewOffset, viewSize});
}

void LogViewerApp::renderLogMessage(const LogMessage &message) {
    static const auto lineBreak = el::CharSet{U'\n'};
    auto lines = el::StringList::fromSplit(message.text, lineBreak);
    auto index = el::ElementIndex{0};
    if (!lines.isEmpty()) {
        renderInitialLine(nextTimestamp(), message.level, lines[index]);
    }
    ++index;
    while (index.isWithin(lines.count())) {
        renderContinuationLine(lines[index]);
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

void LogViewerApp::renderInitialLine(
    const el::StringView &timestamp, const LogLevel level, const el::StringView &text) {
    _logBuffer->setColor(logLevelColor(level));
    auto line = BlockString{timestamp, Color{fg::BrightWhite, bg::Black}};
    line += BlockString{" "_el, Color{fg::White, bg::Black}};
    line += BlockString{logTypeCode(level), logLevelColor(level)};
    line += BlockString{" "_el, logLevelColor(level)};
    line += BlockString{text, logLevelColor(level)};
    _logBuffer->printParagraph(line, initialLineOptions());
}

void LogViewerApp::renderContinuationLine(const el::StringView &text) {
    _logBuffer->setColor(Color{fg::White, bg::Black});
    _logBuffer->printParagraph(BlockString{text, Color{fg::Inherited, bg::Inherited}}, continuationLineOptions());
}

auto LogViewerApp::generateLogMessage() -> LogMessage {
    const auto level = randomLogLevel();
    const auto lengthRoll = random().selectInteger<unsigned int>(1, 100);
    if (lengthRoll <= 80) {
        return LogMessage{level, generateShortMessage(level)};
    }
    if (lengthRoll <= 95) {
        return LogMessage{level, generateLongMessage(level)};
    }
    return LogMessage{level, generateMultilineMessage(level)};
}

auto LogViewerApp::generateShortMessage(const LogLevel level) -> el::StringView {
    static const auto traceFormat = el::StringFormat("{} {} {} on {} with {} for {} in {} ms"_el);
    static const auto infoFormat = el::StringFormat("{} {} completed with 200 for {} in {} ms"_el);
    static const auto warningFormat = el::StringFormat("{} {} {} for {} after {} ms"_el);
    static const auto errorFormat = el::StringFormat("{} {} {} via {}"_el);

    const auto method = random().selectElement(methodChoices());
    const auto route = random().selectElement(routeChoices());
    const auto staticRoute = random().selectElement(staticRouteChoices());
    const auto backend = random().selectElement(backendChoices());
    const auto cache = random().selectElement(cacheChoices());
    const auto ipAddress = randomIpAddress();
    auto message = el::StringBuilder{};
    switch (level) {
    case LogLevel::Trace:
        traceFormat.appendTo(
            message,
            method,
            staticRoute,
            random().selectElement(traceChoices()),
            backend,
            cache,
            ipAddress,
            random().selectInteger<unsigned int>(2, 19));
        break;
    case LogLevel::Info:
        infoFormat.appendTo(message, method, route, ipAddress, random().selectInteger(21, 180));
        break;
    case LogLevel::Warning:
        warningFormat.appendTo(
            message,
            method,
            route,
            random().selectElement(warningChoices()),
            ipAddress,
            random().selectInteger(180, 950));
        break;
    case LogLevel::Error:
        errorFormat.appendTo(message, method, route, random().selectElement(errorChoices()), backend);
        break;
    }
    return message.toString().slice(el::StringSide::Front, el::CpLength{random().selectInteger<unsigned int>(75, 120)});
}

auto LogViewerApp::generateLongMessage(const LogLevel level) -> el::StringView {
    static const auto format = el::StringFormat(
        "; request-id {}; upstream {}; user-agent {}; rate-limit bucket {}; payload {} bytes; cache state {}; "
        "TLS resume {}; compression {}; forwarded-for {}; origin latency {} ms"_el);
    auto message = el::StringBuilder::basedOn(generateShortMessage(level));
    const auto targetLength = el::CpLength{random().selectInteger<unsigned int>(300, 600)};
    while (true) {
        format.appendTo(
            message,
            randomRequestId(),
            random().selectElement(backendChoices()),
            random().selectElement(userAgentChoices()),
            random().selectInteger(1, 16),
            random().selectInteger(820, 64'000),
            random().selectElement(cacheChoices()),
            random().selectElement({"hit"_elv, "miss"_elv}),
            random().selectElement({"brotli"_elv, "gzip"_elv}),
            randomIpAddress(),
            random().selectInteger(8, 430));
        if (message.length() > targetLength && message.length() >= el::CpLength{300}) {
            break;
        }
    }
    return message.toString();
}

auto LogViewerApp::generateMultilineMessage(const LogLevel level) -> el::StringView {
    static const auto format = el::StringFormat{
        "{}\nroute: {}\nrequest-id: {}\nclient: {} via {}\nuser-agent: {}\nextra: retry={}, cache={}, worker={}"_el};
    return format.build(
        generateShortMessage(level),
        random().selectElement(routeChoices()),
        randomRequestId(),
        randomIpAddress(),
        random().selectElement(backendChoices()),
        random().selectElement(userAgentChoices()),
        random().selectInteger(0, 3),
        random().selectElement(cacheChoices()),
        random().selectInteger(1, 24));
}

auto LogViewerApp::nextTimestamp() -> el::StringView {
    const auto days = std::chrono::floor<std::chrono::days>(_logTimestamp);
    const auto ymd = std::chrono::year_month_day{days};
    const auto time = std::chrono::hh_mm_ss{_logTimestamp - days};
    static const auto format = el::StringFormat{"{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}"_el};
    auto result = format.build(
        static_cast<int>(ymd.year()),
        static_cast<unsigned>(ymd.month()),
        static_cast<unsigned>(ymd.day()),
        static_cast<int>(time.hours().count()),
        static_cast<int>(time.minutes().count()),
        static_cast<int>(time.seconds().count()));
    _logTimestamp += randomTimestampStep();
    return result;
}

auto LogViewerApp::randomLogLevel() -> LogLevel {
    const auto roll = random().selectInteger(1, 100);
    if (roll <= 80) {
        return LogLevel::Trace;
    }
    if (roll <= 90) {
        return LogLevel::Info;
    }
    if (roll <= 96) {
        return LogLevel::Warning;
    }
    return LogLevel::Error;
}

auto LogViewerApp::randomDelay() -> std::chrono::milliseconds {
    const auto preset = delayPresets()[_delayPresetIndex];
    return std::chrono::milliseconds{random().selectInteger(preset.minimumMs, preset.maximumMs)};
}

auto LogViewerApp::randomTimestampStep() -> std::chrono::seconds {
    return std::chrono::seconds{random().selectInteger(3, 95)};
}

auto LogViewerApp::randomRequestId() -> el::StringView {
    static const auto format = el::StringFormat{"req-{:08x}"_el};
    const auto value = std::uniform_int_distribution<uint32_t>{0U, 0xffff'ffffU}(_rng);
    return format.build(value);
}

auto LogViewerApp::randomIpAddress() -> el::StringView {
    static const auto format = el::StringFormat{"203.0.113.{}"_el};
    return format.build(random().selectInteger(2, 254));
}

auto LogViewerApp::initialLineOptions() -> const ParagraphOptions & {
    static const auto cOptions = [] {
        auto options = ParagraphOptions{};
        options.setWrappedLineIndent(22);
        options.setLineBreakEndMark(BlockString{U"⤦"_el});
        options.setLineBreakStartMark(BlockString{U"⤥ "_el});
        options.setMaximumLineWraps(2);
        options.setParagraphEllipsisMark(BlockString{"(...)"_el});
        return options;
    }();
    return cOptions;
}

auto LogViewerApp::continuationLineOptions() -> const ParagraphOptions & {
    static const auto cOptions = [] {
        auto options = ParagraphOptions{};
        options.setFirstLineIndent(4);
        options.setWrappedLineIndent(4);
        options.setLineBreakEndMark(BlockString{U"⤦"_el});
        options.setLineBreakStartMark(BlockString{U"⤥ "_el});
        options.setMaximumLineWraps(2);
        options.setParagraphEllipsisMark(BlockString{"(...)"_el});
        return options;
    }();
    return cOptions;
}

auto LogViewerApp::contentRectForBuffer(const BlockSize bufferSize) noexcept -> BlockRectangle {
    return BlockRectangle{
        BlockCoordinate{2},
        BlockCoordinate{3},
        std::max(BlockCoordinate{1}, bufferSize.width() - 4),
        std::max(BlockCoordinate{1}, bufferSize.height() - 6)};
}

auto LogViewerApp::clampViewOffset(
    const BlockPosition offset, const BlockSize viewSize, const BlockSize contentSize) noexcept -> BlockPosition {
    const auto maxX = std::max(BlockCoordinate{0}, contentSize.width() - viewSize.width());
    const auto maxY = std::max(BlockCoordinate{0}, contentSize.height() - viewSize.height());
    return {std::clamp(offset.x(), BlockCoordinate{0}, maxX), std::clamp(offset.y(), BlockCoordinate{0}, maxY)};
}

auto LogViewerApp::logLevelColor(const LogLevel level) noexcept -> Color {
    switch (level) {
    case LogLevel::Trace:
        return {fg::White, bg::Black};
    case LogLevel::Info:
        return {fg::BrightBlue, bg::Black};
    case LogLevel::Warning:
        return {fg::BrightYellow, bg::Black};
    case LogLevel::Error:
        return {fg::BrightRed, bg::Black};
    }
    return Color::reset();
}

auto LogViewerApp::logTypeCode(const LogLevel level) noexcept -> el::StringView {
    switch (level) {
    case LogLevel::Trace:
        return "TRC"_el;
    case LogLevel::Info:
        return "INF"_el;
    case LogLevel::Warning:
        return "WRN"_el;
    case LogLevel::Error:
        return "ERR"_el;
    }
    return "UNK"_el;
}

auto LogViewerApp::delayPresets() noexcept -> std::span<const DelayPreset> {
    static constexpr auto cValues = std::array{
        DelayPreset{10, 100, "10-100 ms"_el},
        DelayPreset{50, 500, "50-500 ms"_el},
        DelayPreset{100, 1000, "100-1000 ms"_el},
        DelayPreset{200, 2000, "200-2000 ms"_el},
    };
    return cValues;
}

auto LogViewerApp::methodChoices() noexcept -> std::span<const el::StringView> {
    static const auto cValues = std::array<const el::StringView, 4>{
        "GET"_el,
        "POST"_el,
        "PUT"_el,
        "DELETE"_el,
    };
    return cValues;
}

auto LogViewerApp::routeChoices() noexcept -> std::span<const el::StringView> {
    static const auto cValues = std::array<const el::StringView, 9>{
        "/"_el,
        "/healthz"_el,
        "/checkout"_el,
        "/api/v1/orders"_el,
        "/api/v1/payments"_el,
        "/api/v1/profile"_el,
        "/admin/reports/daily"_el,
        "/assets/app.bundle.js"_el,
    };
    return cValues;
}

auto LogViewerApp::staticRouteChoices() noexcept -> std::span<const el::StringView> {
    static const auto cValues = std::array<const el::StringView, 5>{
        "/assets/site.css"_el,
        "/assets/app.bundle.js"_el,
        "/images/logo.svg"_el,
        "/fonts/ibm-plex.woff2"_el,
    };
    return cValues;
}

auto LogViewerApp::backendChoices() noexcept -> std::span<const el::StringView> {
    static const auto cValues = std::array<const el::StringView, 6>{
        "edge-gw-1"_el,
        "edge-gw-2"_el,
        "api-eu-1"_el,
        "api-eu-2"_el,
        "payments-us-1"_el,
        "search-eu-3"_el,
    };
    return cValues;
}

auto LogViewerApp::cacheChoices() noexcept -> std::span<const el::StringView> {
    static const auto cValues = std::array<const el::StringView, 5>{
        "warm cache"_el,
        "cold cache"_el,
        "stale-if-error"_el,
        "revalidated cache"_el,
    };
    return cValues;
}

auto LogViewerApp::userAgentChoices() noexcept -> std::span<const el::StringView> {
    static const auto cValues = std::array<const el::StringView, 4>{
        "Mozilla/5.0 Chrome/135"_el,
        "curl/8.9.1"_el,
        "Firefox/139.0"_el,
        "HealthChecker/2.4"_el,
    };
    return cValues;
}

auto LogViewerApp::warningChoices() noexcept -> std::span<const el::StringView> {
    static const auto cValues = std::array<const el::StringView, 4>{
        "needed a retry"_el,
        "hit the slow-path cache refresh"_el,
        "waited for an upstream reconnect"_el,
        "served stale content while the origin recovered"_el,
    };
    return cValues;
}

auto LogViewerApp::errorChoices() noexcept -> std::span<const el::StringView> {
    static const auto cValues = std::array<const el::StringView, 4>{
        "failed with 502 after upstream reset"_el,
        "failed with 503 because all workers were busy"_el,
        "aborted after TLS negotiation failed"_el,
        "returned 500 after the session store timed out"_el,
    };
    return cValues;
}

auto LogViewerApp::traceChoices() noexcept -> std::span<const el::StringView> {
    static const auto cValues = std::array<const el::StringView, 5>{
        "header normalization complete"_el,
        "route candidate matched"_el,
        "gzip dictionary selected"_el,
        "session cookie decoded"_el,
    };
    return cValues;
}

}
