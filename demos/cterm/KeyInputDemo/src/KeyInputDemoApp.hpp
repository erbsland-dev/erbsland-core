// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <TerminalApplication.hpp>

#include <array>
#include <chrono>
#include <random>

namespace demo {

/// Interactive demo that visualizes detected key presses in a horizontally scrolling field.
class KeyInputDemoApp final : public TerminalApplication {
public:
    using TerminalApplication::TerminalApplication;

public:
    /// Prepare the shared terminal update settings before the terminal is initialized.
    void beforeInitialize() override;
    /// Initialize the scrolling field once after the terminal is ready.
    auto beforeMain() -> int override;
    /// Handle key stamping and the Escape quit key.
    void onKey(const Key &key) override;
    /// Render the scrolling field into the shared demo buffer.
    void onRenderToBuffer() override;
    /// Match the original scroll cadence for input polling and frame updates.
    [[nodiscard]] auto loopInterval() const noexcept -> std::chrono::milliseconds override { return cScrollDelay; }

private:
    [[nodiscard]] auto canvasSize() noexcept -> Size;
    [[nodiscard]] auto fieldRectForCanvas(Size canvasSize) const noexcept -> Rectangle;
    [[nodiscard]] auto visibleFieldSize() noexcept -> Size;
    void initializeScrollBuffer() noexcept;
    void advanceScroll() noexcept;
    void stampKeyBlock(const Key &key) noexcept;
    void drawHeader(Rectangle rect);
    void drawField(Rectangle rect);
    void drawFooter(Rectangle rect);
    [[nodiscard]] auto footerText() const -> BlockString;

private:
    [[nodiscard]] static auto backgroundChar() noexcept -> Block;
    [[nodiscard]] static auto guideColumnChar() noexcept -> Block;
    [[nodiscard]] static auto stampColors() noexcept -> const std::array<Color, 10> &;

private:
    constexpr static auto cScrollBufferSize = Size{Coordinate{250}, Coordinate{30}};
    constexpr static auto cScrollDelay = std::chrono::milliseconds{200};

private:
    RemappedBuffer _scrollBuffer{cScrollBufferSize, Orientation::Horizontal, backgroundChar()};
    std::mt19937 _random{std::random_device{}()};
    std::size_t _insertedColumnCount{0};
    bool _firstFrame{true};
};

}
