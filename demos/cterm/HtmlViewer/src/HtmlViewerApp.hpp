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
    [[nodiscard]] auto canvasSize() noexcept -> Size;
    auto printRenderedDocument() -> int;
    void loadDocument();
    void renderDocumentIfRequired(Coordinate contentWidth);
    void drawHeader(Rectangle rect);
    void drawFooter(Rectangle rect);
    void drawDocument(Rectangle rect);
    void updateView(Size viewSize) noexcept;
    void advanceDocumentStylePreset() noexcept;
    [[nodiscard]] auto locationText() const -> el::String;
    [[nodiscard]] auto displayName() const -> el::String;
    [[nodiscard]] auto documentStylePresetName() const noexcept -> el::String;
    [[nodiscard]] static auto parseDocumentStylePreset(el::String value, DocumentStylePreset &preset) noexcept -> bool;
    [[nodiscard]] static auto contentRectForBuffer(Size bufferSize) noexcept -> Rectangle;
    [[nodiscard]] static auto clampViewOffset(Coordinate offset, Coordinate viewHeight, Coordinate height) noexcept
        -> Coordinate;

private:
    el::Path _htmlFilePath;
    el::String _html;
    el::TextDocument _document;
    DocumentStylePreset _documentStylePreset{DocumentStylePreset::Styled};
    bool _printMode{false};
    std::shared_ptr<CursorBuffer> _documentBuffer = std::make_shared<CursorBuffer>(
        Size{80, 1}, CursorBuffer::OverflowMode::ExpandThenShift, Size{80, 20'000}, Block{U' ', fg::White, bg::Black});
    BufferView _documentView{_documentBuffer, Rectangle{Coordinate{0}, Coordinate{0}, Coordinate{1}, Coordinate{1}}};
    Coordinate _viewOffsetY{0};
    Coordinate _renderedContentWidth{0};
    bool _documentDirty{true};
};

}
