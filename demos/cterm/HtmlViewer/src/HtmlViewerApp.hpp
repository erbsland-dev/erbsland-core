// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/cterm/TerminalDocumentRenderer.hpp>
#include <erbsland/path/Path.hpp>
#include <erbsland/text/html/HtmlParser.hpp>
#include <TerminalApplication.hpp>

#include <memory>

namespace demo {

/// Demonstrate HTML parsing into `TextDocument` and terminal rendering into a scrollable buffer.
class HtmlViewerApp final : public TerminalApplication {
public:
    using TerminalApplication::TerminalApplication;

protected: // implement Application
    /// Register command line options for selecting print mode, style, and the optional input file.
    void registerCommandLineOptions(const el::OptionsPtr &options) override;

public: // implement TerminalApplication
    /// Prepare the terminal minimum-size message and the document buffer before terminal initialization.
    void beforeInitialize() override;
    /// Load and parse the selected HTML file, then either print it or enter the interactive viewer.
    auto beforeMain() -> int override;
    /// Handle viewport navigation and style switching keys.
    void onKey(const Key &key) override;
    /// Render the header, document viewport, and footer.
    void onRenderToBuffer() override;

private:
    using DocumentStylePreset = TerminalDocumentStyle::Predefined;

private:
    [[nodiscard]] auto canvasSize() noexcept -> BlockSize;
    auto printRenderedDocument() -> int;
    void loadDocument();
    void renderDocumentIfRequired(BlockCoordinate contentWidth);
    void drawHeader(BlockRectangle rect);
    void drawFooter(BlockRectangle rect);
    void drawDocument(BlockRectangle rect);
    void updateView(BlockSize viewSize) noexcept;
    void advanceDocumentStylePreset() noexcept;
    [[nodiscard]] auto locationText() const -> el::String;
    [[nodiscard]] auto displayName() const -> el::String;
    [[nodiscard]] auto documentStylePresetName() const noexcept -> el::String;
    [[nodiscard]] static auto parseDocumentStylePreset(el::String value, DocumentStylePreset &preset) noexcept -> bool;
    [[nodiscard]] static auto contentRectForBuffer(BlockSize bufferSize) noexcept -> BlockRectangle;
    [[nodiscard]] static auto clampViewOffset(
        BlockCoordinate offset, BlockCoordinate viewHeight, BlockCoordinate height) noexcept -> BlockCoordinate;

private:
    el::Path _htmlFilePath;
    el::String _html;
    el::TextDocument _document;
    DocumentStylePreset _documentStylePreset{DocumentStylePreset::Styled};
    bool _printMode{false};
    std::shared_ptr<CursorBuffer> _documentBuffer = std::make_shared<CursorBuffer>(
        BlockSize{80, 1},
        CursorBuffer::OverflowMode::ExpandThenShift,
        BlockSize{80, 20'000},
        Block{U' ', fg::White, bg::Black});
    BufferView _documentView{
        _documentBuffer,
        BlockRectangle{BlockCoordinate{0}, BlockCoordinate{0}, BlockCoordinate{1}, BlockCoordinate{1}}};
    BlockCoordinate _viewOffsetY{0};
    BlockCoordinate _renderedContentWidth{0};
    bool _documentDirty{true};
};

}
