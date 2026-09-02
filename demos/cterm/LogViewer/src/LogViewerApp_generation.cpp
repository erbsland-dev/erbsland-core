// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "LogViewerApp.hpp"

#include <algorithm>
#include <array>
#include <utility>

namespace demo {
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

auto LogViewerApp::generateShortMessage(const LogLevel level) -> el::String {
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
    auto message = el::AnyStringBuilder{};
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

auto LogViewerApp::generateLongMessage(const LogLevel level) -> el::String {
    static const auto format = el::StringFormat(
        "; request-id {}; upstream {}; user-agent {}; rate-limit bucket {}; payload {} bytes; cache state {}; "
        "TLS resume {}; compression {}; forwarded-for {}; origin latency {} ms"_el);
    auto message = el::AnyStringBuilder::basedOn(generateShortMessage(level));
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
            random().selectElement({el::String{"hit"_el}, el::String{"miss"_el}}),
            random().selectElement({el::String{"brotli"_el}, el::String{"gzip"_el}}),
            randomIpAddress(),
            random().selectInteger(8, 430));
        if (message.length() > targetLength && message.length() >= el::CpLength{300}) {
            break;
        }
    }
    return message.toString();
}

auto LogViewerApp::generateMultilineMessage(const LogLevel level) -> el::String {
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

auto LogViewerApp::nextTimestamp() -> el::String {
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

auto LogViewerApp::randomRequestId() -> el::String {
    static const auto format = el::StringFormat{"req-{:08x}"_el};
    const auto value = std::uniform_int_distribution<uint32_t>{0U, 0xffff'ffffU}(_rng);
    return format.build(value);
}

auto LogViewerApp::randomIpAddress() -> el::String {
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

auto LogViewerApp::contentRectForBuffer(const Size bufferSize) noexcept -> Rectangle {
    return Rectangle{
        Coordinate{2},
        Coordinate{3},
        std::max(Coordinate{1}, bufferSize.width() - 4),
        std::max(Coordinate{1}, bufferSize.height() - 6)};
}

auto LogViewerApp::clampViewOffset(const Position offset, const Size viewSize, const Size contentSize) noexcept
    -> Position {
    const auto maxX = std::max(Coordinate{0}, contentSize.width() - viewSize.width());
    const auto maxY = std::max(Coordinate{0}, contentSize.height() - viewSize.height());
    return {std::clamp(offset.x(), Coordinate{0}, maxX), std::clamp(offset.y(), Coordinate{0}, maxY)};
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

auto LogViewerApp::logTypeCode(const LogLevel level) noexcept -> el::String {
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

auto LogViewerApp::methodChoices() noexcept -> std::span<const el::String> {
    static const auto cValues = std::array<const el::String, 4>{
        "GET"_el,
        "POST"_el,
        "PUT"_el,
        "DELETE"_el,
    };
    return cValues;
}

auto LogViewerApp::routeChoices() noexcept -> std::span<const el::String> {
    static const auto cValues = std::array<const el::String, 9>{
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

auto LogViewerApp::staticRouteChoices() noexcept -> std::span<const el::String> {
    static const auto cValues = std::array<const el::String, 5>{
        "/assets/site.css"_el,
        "/assets/app.bundle.js"_el,
        "/images/logo.svg"_el,
        "/fonts/ibm-plex.woff2"_el,
    };
    return cValues;
}

auto LogViewerApp::backendChoices() noexcept -> std::span<const el::String> {
    static const auto cValues = std::array<const el::String, 6>{
        "edge-gw-1"_el,
        "edge-gw-2"_el,
        "api-eu-1"_el,
        "api-eu-2"_el,
        "payments-us-1"_el,
        "search-eu-3"_el,
    };
    return cValues;
}

auto LogViewerApp::cacheChoices() noexcept -> std::span<const el::String> {
    static const auto cValues = std::array<const el::String, 5>{
        "warm cache"_el,
        "cold cache"_el,
        "stale-if-error"_el,
        "revalidated cache"_el,
    };
    return cValues;
}

auto LogViewerApp::userAgentChoices() noexcept -> std::span<const el::String> {
    static const auto cValues = std::array<const el::String, 4>{
        "Mozilla/5.0 Chrome/135"_el,
        "curl/8.9.1"_el,
        "Firefox/139.0"_el,
        "HealthChecker/2.4"_el,
    };
    return cValues;
}

auto LogViewerApp::warningChoices() noexcept -> std::span<const el::String> {
    static const auto cValues = std::array<const el::String, 4>{
        "needed a retry"_el,
        "hit the slow-path cache refresh"_el,
        "waited for an upstream reconnect"_el,
        "served stale content while the origin recovered"_el,
    };
    return cValues;
}

auto LogViewerApp::errorChoices() noexcept -> std::span<const el::String> {
    static const auto cValues = std::array<const el::String, 4>{
        "failed with 502 after upstream reset"_el,
        "failed with 503 because all workers were busy"_el,
        "aborted after TLS negotiation failed"_el,
        "returned 500 after the session store timed out"_el,
    };
    return cValues;
}

auto LogViewerApp::traceChoices() noexcept -> std::span<const el::String> {
    static const auto cValues = std::array<const el::String, 5>{
        "header normalization complete"_el,
        "route candidate matched"_el,
        "gzip dictionary selected"_el,
        "session cookie decoded"_el,
    };
    return cValues;
}

}
