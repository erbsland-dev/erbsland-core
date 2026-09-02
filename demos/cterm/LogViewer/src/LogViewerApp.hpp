// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <TerminalApplication.hpp>

#include <chrono>
#include <memory>
#include <random>
#include <span>
#include <string>

namespace demo {

/// Demonstrate a live log viewport backed by a growing cursor buffer.
class LogViewerApp final : public TerminalApplication {
public:
    using TerminalApplication::TerminalApplication;

public:
    /// Prepare the shared terminal update settings before the terminal is initialized.
    void beforeInitialize() override;
    /// Seed the first simulated log event after the terminal is ready.
    auto beforeMain() -> int override;
    /// Handle viewport navigation and playback control keys.
    void onKey(const Key &key) override;
    /// Render the current viewport of the simulated log stream.
    void onRenderToBuffer() override;
    /// Keep the shared render loop responsive enough for the live log feed.
    [[nodiscard]] auto loopInterval() const noexcept -> std::chrono::milliseconds override {
        return std::chrono::milliseconds{50};
    }

private:
    enum class LogLevel : uint8_t {
        Trace,
        Info,
        Warning,
        Error,
    };

    struct LogMessage final {
        LogLevel level{};
        el::String text;
    };

    struct DelayPreset final {
        int minimumMs{};
        int maximumMs{};
        el::StringLiteral label;
    };

private:
    [[nodiscard]] auto canvasSize() noexcept -> Size;
    void adjustDelayPreset(int delta) noexcept;
    void scheduleNextMessage() noexcept;
    void appendGeneratedMessage();
    void drawHeader(Rectangle rect);
    void drawFooter(Rectangle rect);
    void drawLogView(Rectangle rect);
    void updateView(Size viewSize) noexcept;
    void renderLogMessage(const LogMessage &message);
    [[nodiscard]] static auto shouldCopyCell(const Block &cell) noexcept -> bool;
    void renderInitialLine(const el::String &timestamp, LogLevel level, const el::String &text);
    void renderContinuationLine(const el::String &text);
    [[nodiscard]] auto generateLogMessage() -> LogMessage;
    [[nodiscard]] auto generateShortMessage(LogLevel level) -> el::String;
    [[nodiscard]] auto generateLongMessage(LogLevel level) -> el::String;
    [[nodiscard]] auto generateMultilineMessage(LogLevel level) -> el::String;
    [[nodiscard]] auto nextTimestamp() -> el::String;
    [[nodiscard]] auto randomLogLevel() -> LogLevel;
    [[nodiscard]] auto randomDelay() -> std::chrono::milliseconds;
    [[nodiscard]] auto randomTimestampStep() -> std::chrono::seconds;
    [[nodiscard]] auto randomRequestId() -> el::String;
    [[nodiscard]] auto randomIpAddress() -> el::String;
    [[nodiscard]] static auto initialLineOptions() -> const ParagraphOptions &;
    [[nodiscard]] static auto continuationLineOptions() -> const ParagraphOptions &;
    [[nodiscard]] static auto contentRectForBuffer(Size bufferSize) noexcept -> Rectangle;
    [[nodiscard]] static auto clampViewOffset(Position offset, Size viewSize, Size contentSize) noexcept -> Position;
    [[nodiscard]] static auto logLevelColor(LogLevel level) noexcept -> Color;
    [[nodiscard]] static auto logTypeCode(LogLevel level) noexcept -> el::String;
    [[nodiscard]] static auto delayPresets() noexcept -> std::span<const DelayPreset>;
    [[nodiscard]] static auto methodChoices() noexcept -> std::span<const el::String>;
    [[nodiscard]] static auto routeChoices() noexcept -> std::span<const el::String>;
    [[nodiscard]] static auto staticRouteChoices() noexcept -> std::span<const el::String>;
    [[nodiscard]] static auto backendChoices() noexcept -> std::span<const el::String>;
    [[nodiscard]] static auto cacheChoices() noexcept -> std::span<const el::String>;
    [[nodiscard]] static auto userAgentChoices() noexcept -> std::span<const el::String>;
    [[nodiscard]] static auto warningChoices() noexcept -> std::span<const el::String>;
    [[nodiscard]] static auto errorChoices() noexcept -> std::span<const el::String>;
    [[nodiscard]] static auto traceChoices() noexcept -> std::span<const el::String>;

private:
    std::shared_ptr<CursorBuffer> _logBuffer = std::make_shared<CursorBuffer>(
        Size{Coordinate{250}, Coordinate{10}},
        CursorBuffer::OverflowMode::ExpandThenShift,
        Size{Coordinate{250}, Coordinate{500}},
        Block{U' ', fg::Default, bg::Black});
    BufferView _logView{_logBuffer, Rectangle{Coordinate{0}, Coordinate{0}, Coordinate{1}, Coordinate{1}}};
    std::mt19937 _rng{std::random_device{}()};
    std::chrono::steady_clock::time_point _nextMessageAt{};
    std::chrono::sys_seconds _logTimestamp{
        std::chrono::sys_days{std::chrono::year{2026} / std::chrono::March / 26} + std::chrono::hours{9}};
    Position _viewOffset{0, 0};
    std::size_t _messageCount{0};
    std::size_t _delayPresetIndex{2};
    bool _followMode{true};
};

}
