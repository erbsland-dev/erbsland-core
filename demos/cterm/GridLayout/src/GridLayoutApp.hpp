// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <TerminalApplication.hpp>

#include <array>
#include <vector>

namespace demo {

/// Demonstrate `FrameBorder` and `GridLayout` with independently configurable border elements.
class GridLayoutApp final : public TerminalApplication {
public:
    using TerminalApplication::TerminalApplication;

public:
    /// Configure the minimum terminal size for the grid layout demo.
    void beforeInitialize() override;
    /// Handle keys that cycle border elements and grid dimensions.
    void onKey(const Key &key) override;
    /// Render the current grid layout state into the shared demo buffer.
    void onRenderToBuffer() override;

private:
    struct BorderElementInfo final {
        FrameBorderElement element;
        char key;
        el::String label;
    };

private:
    void cycleBorder(FrameBorderElement element) noexcept;
    void renderGrid(BlockRectangle gridArea);
    void renderCellContent(const GridLayout &layout, BlockPosition origin);
    void renderStatus(BlockRectangle statusRect);
    void appendBorderStatusLine(BlockStringEditor &status, std::size_t begin, std::size_t end) const;
    void renderFooter(BlockRectangle footerRect);
    [[nodiscard]] auto createLayout(BlockSize availableSize) const -> GridLayout;
    [[nodiscard]] auto borderLineSize(FrameBorderElement element) const noexcept -> BlockCoordinate;
    [[nodiscard]] static auto nextStyle(FrameStyle style) noexcept -> FrameStyle;
    [[nodiscard]] static auto styleName(FrameStyle style) noexcept -> el::String;
    [[nodiscard]] static auto borderElements() noexcept -> const std::array<BorderElementInfo, 6> &;
    [[nodiscard]] static auto distribute(BlockCoordinate total, std::size_t count) -> std::vector<BlockCoordinate>;

private:
    FrameBorder _border{FrameStyle::Light, Color{fg::BrightWhite, bg::Black}};
    std::size_t _columnCount{3};
    std::size_t _rowCount{2};
};

}
