// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <TerminalApplication.hpp>

#include <array>

namespace demo {

/// Interactive demo showing the bitmap rendering modes and options.
class BitmapShowcaseApp final : public TerminalApplication {
public:
    using TerminalApplication::TerminalApplication;

public:
    /// Prepare the shared terminal update settings before the terminal is initialized.
    void beforeInitialize() override;
    /// Handle page navigation and variant selection keys.
    void onKey(const Key &key) override;
    /// Render the current showcase page into the shared demo buffer.
    void onRenderToBuffer() override;

private:
    void drawSelector(BlockRectangle rect);
    void drawPreview(BlockRectangle rect);
    void drawScaleModeVariant(BlockRectangle rect, std::size_t variantIndex);
    void drawColorModeVariant(BlockRectangle rect, std::size_t variantIndex);
    void drawLayoutVariant(BlockRectangle rect, std::size_t variantIndex);
    void drawStyleVariant(BlockRectangle rect, std::size_t variantIndex);
    void drawPreviewPanel(BlockRectangle rect, el::StringView title, Color fillColor);
    void drawFooter(BlockRectangle rect);
    [[nodiscard]] auto footerText() const -> BlockString;
    [[nodiscard]] auto pageTitle() const -> el::StringView;
    [[nodiscard]] auto variantCount(std::size_t pageIndex) const noexcept -> std::size_t;
    [[nodiscard]] auto variantTitle(std::size_t pageIndex, std::size_t variantIndex) const -> el::StringView;
    [[nodiscard]] auto selectedVariantIndex() const noexcept -> std::size_t;
    void selectVariantDelta(int delta) noexcept;

private:
    [[nodiscard]] static auto ringBitmap() -> const Bitmap &;
    [[nodiscard]] static auto rocketBitmap() -> const Bitmap &;
    [[nodiscard]] static auto waveBitmap() -> const Bitmap &;
    [[nodiscard]] static auto circuitBitmap() -> const Bitmap &;
    [[nodiscard]] static auto rainbowColors() -> const ColorSequence &;

private:
    std::size_t _pageIndex{0};
    std::array<std::size_t, 4> _selectedVariantByPage{};
};

}
