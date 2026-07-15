// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <TerminalApplication.hpp>

#include <chrono>

namespace demo {

/// Browse several text rendering demonstrations with left and right cursor keys.
class TextGalleryApp final : public TerminalApplication {
public:
    using TerminalApplication::TerminalApplication;

public:
    /// Prepare the shared terminal update settings before the terminal is initialized.
    void beforeInitialize() override;
    /// Load the shared bitmap font after the terminal is ready.
    auto beforeMain() -> int override;
    /// Handle gallery page navigation keys.
    void onKey(const Key &key) override;
    /// Render the current gallery page into the shared demo buffer.
    void onRenderToBuffer() override;

private:
    void drawOverviewPage(BlockRectangle contentRect);
    void drawMixedWidthPage(BlockRectangle contentRect);
    void drawBitmapFontPage(BlockRectangle contentRect);
    void drawPanel(
        BlockRectangle rect,
        el::StringView title,
        el::StringView text,
        Alignment alignment,
        FrameStyle frameStyle,
        Color fillColor,
        Color textColor);
    void drawFooter(BlockRectangle rect);
    [[nodiscard]] auto buildFooterText() const -> BlockString;
    [[nodiscard]] static auto titleColors() -> ColorSequence;
    [[nodiscard]] static auto titleForWidth(BlockCoordinate width) -> el::StringView;

private:
    FontPtr _font;
    std::size_t _pageIndex{0};
};

}
