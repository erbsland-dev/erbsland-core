// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <TerminalApplication.hpp>

#include <chrono>
#include <random>
#include <vector>

/// Continuously add random frames to demonstrate automatic line combinations.
class FrameWeaverApp final : public TerminalApplication {
public:
    using TerminalApplication::TerminalApplication;

public:
    /// Prepare the shared terminal update settings before the terminal is initialized.
    void beforeInitialize() override;
    /// Initialize the animation clock after the terminal is ready.
    auto beforeMain() -> int override;
    /// Handle animation control keys.
    void onKey(const Key &key) override;
    /// Render the animated frame composition into the shared demo buffer.
    void onRenderToBuffer() override;
    /// Slow the shared render loop slightly to match the original demo pacing.
    [[nodiscard]] auto loopInterval() const noexcept -> std::chrono::milliseconds override {
        return std::chrono::milliseconds{100};
    }

private:
    enum class StyleMode : uint8_t {
        Light,
        Double,
        Heavy,
        Mixed,
        Block,
        Custom,
        All,
    };

    struct FrameSpec final {
        double x{};
        double y{};
        double width{};
        double height{};
        FrameStyle style = FrameStyle::Light;
        Block16StylePtr customStyle{};
        Color color{};
    };

private:
    void updateAnimation(std::chrono::milliseconds elapsed) noexcept;
    void addFrame() noexcept;
    void renderFrames(BlockRectangle contentRect);
    [[nodiscard]] auto createRandomFrame() -> FrameSpec;
    [[nodiscard]] static auto frameRectangle(FrameSpec frame, BlockRectangle contentRect) -> BlockRectangle;
    [[nodiscard]] static auto prismFrameStyle() -> const Block16StylePtr &;
    [[nodiscard]] static auto colors() -> const ColorSequence &;
    [[nodiscard]] auto availableStyles() const -> std::vector<FrameSpec>;
    [[nodiscard]] auto modeName() const -> el::StringView;
    [[nodiscard]] auto buildPrompt() const -> BlockString;

private:
    std::mt19937 _rng{std::random_device{}()};
    std::vector<FrameSpec> _frames;
    std::chrono::milliseconds _frameDelay{1000};
    std::chrono::milliseconds _accumulator{};
    std::chrono::steady_clock::time_point _lastTick{std::chrono::steady_clock::now()};
    StyleMode _styleMode{StyleMode::Heavy};
};
