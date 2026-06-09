// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PlasmaRenderer.hpp"

#include <TerminalApplication.hpp>

#include <chrono>

/// Run the animated plasma buffer demo.
class RetroPlasmaApp final : public TerminalApplication {
public:
    using TerminalApplication::TerminalApplication;

public:
    /// Prepare the shared terminal update settings before the terminal is initialized.
    void beforeInitialize() override;
    /// Initialize the frame timer after the terminal is ready.
    auto beforeMain() -> int override;
    /// Handle animation control keys.
    void onKey(const Key &key) override;
    /// Render the plasma effect into the shared demo buffer.
    void onRenderToBuffer() override;
    /// Match the original higher refresh cadence of the plasma animation.
    [[nodiscard]] auto loopInterval() const noexcept -> std::chrono::milliseconds override {
        return std::chrono::milliseconds{35};
    }

private:
    void drawPrompt() noexcept;
    [[nodiscard]] auto buildPrompt() const -> BlockString;

private:
    PlasmaRenderer _renderer;
    double _phase{0.0};
    double _speed{1.0};
    std::size_t _paletteIndex{0};
    bool _paused{false};
    std::chrono::steady_clock::time_point _lastFrameTime{std::chrono::steady_clock::now()};
};
