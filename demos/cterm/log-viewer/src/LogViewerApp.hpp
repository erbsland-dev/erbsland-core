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
        el::StringView text;
    };

    struct DelayPreset final {
        int minimumMs{};
        int maximumMs{};
        el::StringLiteral label;
    };

private:
    [[nodiscard]] auto canvasSize() noexcept -> BlockSize;
    void adjustDelayPreset(int delta) noexcept;
    void scheduleNextMessage() noexcept;
    void appendGeneratedMessage();
    void drawHeader(BlockRectangle rect);
    void drawFooter(BlockRectangle rect);
    void drawLogView(BlockRectangle rect);
    void updateView(BlockSize viewSize) noexcept;
    void renderLogMessage(const LogMessage &message);
    [[nodiscard]] static auto shouldCopyCell(const Block &cell) noexcept -> bool;
    void renderInitialLine(const el::StringView &timestamp, LogLevel level, const el::StringView &text);
    void renderContinuationLine(const el::StringView &text);
    [[nodiscard]] auto generateLogMessage() -> LogMessage;
    [[nodiscard]] auto generateShortMessage(LogLevel level) -> el::StringView;
    [[nodiscard]] auto generateLongMessage(LogLevel level) -> el::StringView;
    [[nodiscard]] auto generateMultilineMessage(LogLevel level) -> el::StringView;
    [[nodiscard]] auto nextTimestamp() -> el::StringView;
    [[nodiscard]] auto randomLogLevel() -> LogLevel;
    [[nodiscard]] auto randomDelay() -> std::chrono::milliseconds;
    [[nodiscard]] auto randomTimestampStep() -> std::chrono::seconds;
    [[nodiscard]] auto randomRequestId() -> el::StringView;
    [[nodiscard]] auto randomIpAddress() -> el::StringView;
    [[nodiscard]] static auto initialLineOptions() -> const ParagraphOptions &;
    [[nodiscard]] static auto continuationLineOptions() -> const ParagraphOptions &;
    [[nodiscard]] static auto contentRectForBuffer(BlockSize bufferSize) noexcept -> BlockRectangle;
    [[nodiscard]] static auto clampViewOffset(BlockPosition offset, BlockSize viewSize, BlockSize contentSize) noexcept
        -> BlockPosition;
    [[nodiscard]] static auto logLevelColor(LogLevel level) noexcept -> Color;
    [[nodiscard]] static auto logTypeCode(LogLevel level) noexcept -> el::StringView;
    [[nodiscard]] static auto delayPresets() noexcept -> std::span<const DelayPreset>;
    [[nodiscard]] static auto methodChoices() noexcept -> std::span<const el::StringView>;
    [[nodiscard]] static auto routeChoices() noexcept -> std::span<const el::StringView>;
    [[nodiscard]] static auto staticRouteChoices() noexcept -> std::span<const el::StringView>;
    [[nodiscard]] static auto backendChoices() noexcept -> std::span<const el::StringView>;
    [[nodiscard]] static auto cacheChoices() noexcept -> std::span<const el::StringView>;
    [[nodiscard]] static auto userAgentChoices() noexcept -> std::span<const el::StringView>;
    [[nodiscard]] static auto warningChoices() noexcept -> std::span<const el::StringView>;
    [[nodiscard]] static auto errorChoices() noexcept -> std::span<const el::StringView>;
    [[nodiscard]] static auto traceChoices() noexcept -> std::span<const el::StringView>;

private:
    std::shared_ptr<CursorBuffer> _logBuffer = std::make_shared<CursorBuffer>(
        BlockSize{BlockCoordinate{250}, BlockCoordinate{10}},
        CursorBuffer::OverflowMode::ExpandThenShift,
        BlockSize{BlockCoordinate{250}, BlockCoordinate{500}},
        Block{U' ', fg::Default, bg::Black});
    BufferView _logView{
        _logBuffer, BlockRectangle{BlockCoordinate{0}, BlockCoordinate{0}, BlockCoordinate{1}, BlockCoordinate{1}}};
    std::mt19937 _rng{std::random_device{}()};
    std::chrono::steady_clock::time_point _nextMessageAt{};
    std::chrono::sys_seconds _logTimestamp{
        std::chrono::sys_days{std::chrono::year{2026} / std::chrono::March / 26} + std::chrono::hours{9}};
    BlockPosition _viewOffset{0, 0};
    std::size_t _messageCount{0};
    std::size_t _delayPresetIndex{2};
    bool _followMode{true};
};

}
