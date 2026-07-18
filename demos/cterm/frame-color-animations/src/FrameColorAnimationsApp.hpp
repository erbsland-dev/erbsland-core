// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <TerminalApplication.hpp>

#include <array>

namespace demo {

/// Interactive demo showing the animated frame color modes.
class FrameColorAnimationsApp final : public TerminalApplication {
public:
    using TerminalApplication::TerminalApplication;

public:
    /// Prepare the shared terminal update settings before the terminal is initialized.
    void beforeInitialize() override;
    /// Render the animated frame showcase into the shared demo buffer.
    void onRenderToBuffer() override;

private:
    struct PanelSpec final {
        el::String title;
        FrameStyle style;
        FrameColorMode mode;
        std::size_t sequenceIndex;
    };

private:
    void drawPanel(BlockRectangle rect, const PanelSpec &panel);
    void drawHeader(BlockRectangle rect);
    void drawFooter(BlockRectangle rect);

private:
    [[nodiscard]] static auto panelSpecs() -> const std::array<PanelSpec, 7> &;
    [[nodiscard]] static auto colorSequence(std::size_t index) -> const ColorSequence &;
    [[nodiscard]] static auto outerFrameColors() -> const ColorSequence &;
    [[nodiscard]] static auto fillColors() -> const ColorSequence &;
};

}
