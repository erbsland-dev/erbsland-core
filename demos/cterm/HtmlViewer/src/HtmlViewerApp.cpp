// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "HtmlViewerApp.hpp"

#include <algorithm>

namespace demo {

void HtmlViewerApp::registerCommandLineOptions(const el::OptionsPtr &options) {
    options->addOption({"-p"_el, "--print"_el, "print"_el})
        .setHelp("Render the HTML document once and write it to standard output."_el);
    options->addOption({"-s"_el, "--style"_el, "style"_el})
        .setChoices(el::OptionChoices::create({"plain"_el, "simple"_el, "styled"_el}))
        .setDefaultValue("styled"_el)
        .setHelp("Select the initial document style."_el);
    options->addOption("html-file"_el).setHelp("The HTML file to view. Uses a bundled demo document when omitted."_el);
}

void HtmlViewerApp::beforeInitialize() {
    info().setApplicationName("HTML Viewer"_el);
    _updateSettings.setMinimumSize(BlockSize{BlockCoordinate{60}, BlockCoordinate{12}});
    _updateSettings.setMinimumSizeBackground(Block{U' ', bg::Black});
    _updateSettings.setMinimumSizeMessage(
        BlockString{
            "Resize the terminal to at least 60x12 cells for the HTML viewer."_el, Color{fg::BrightWhite, bg::Black}});
    _documentBuffer = std::make_shared<CursorBuffer>(
        BlockSize{80, 1},
        CursorBuffer::OverflowMode::ExpandThenShift,
        BlockSize{80, 20'000},
        Block{U' ', fg::White, bg::Black});
    _documentView.setContent(_documentBuffer);
}

auto HtmlViewerApp::beforeMain() -> int {
    _printMode = optionValues()->getFlag("print"_el);
    const auto styleName = optionValues()->getText("style"_el, "styled"_el);
    if (!parseDocumentStylePreset(styleName, _documentStylePreset)) {
        _documentStylePreset = DocumentStylePreset::Styled;
    }

    const auto htmlFileName = optionValues()->getText("html-file"_el);
    _htmlFilePath = htmlFileName.isEmpty()
        ? el::Path::fromNativeOrThrow(el::StringLiteral(ERBSLAND_CORE_HTML_VIEWER_DEFAULT_FILE))
        : el::Path::fromNativeOrThrow(htmlFileName);
    loadDocument();
    if (_printMode || !terminal()->isInteractive()) {
        return printRenderedDocument();
    }
    return 0;
}

auto HtmlViewerApp::canvasSize() noexcept -> BlockSize {
    if (_buffer.size().isZero()) {
        return terminal()->size().expandedWith(_updateSettings.minimumSize());
    }
    return _buffer.size();
}

auto HtmlViewerApp::printRenderedDocument() -> int {
    renderDocumentIfRequired(std::max(terminal()->size().width(), BlockCoordinate{1}));
    terminal()->write(*_documentBuffer);
    terminal()->flush();
    _quitRequested = true;
    return 0;
}

void HtmlViewerApp::loadDocument() {
    _html = _htmlFilePath.content().readTextOrThrow();
    _document = el::html::HtmlParser{_html}.parse();
    _documentDirty = true;
}

void HtmlViewerApp::onKey(const Key &key) {
    const auto contentRect = contentRectForBuffer(canvasSize());
    const auto pageStep = std::max(contentRect.height() - BlockCoordinate{1}, BlockCoordinate{1});
    const auto contentHeight = _documentBuffer->size().height();
    if (key == U'q' || key == Key::Escape) {
        _quitRequested = true;
    } else if (key == Key::Up) {
        _viewOffsetY = clampViewOffset(_viewOffsetY - BlockCoordinate{1}, contentRect.height(), contentHeight);
    } else if (key == Key::Down) {
        _viewOffsetY = clampViewOffset(_viewOffsetY + BlockCoordinate{1}, contentRect.height(), contentHeight);
    } else if (key == Key::PageUp) {
        _viewOffsetY = clampViewOffset(_viewOffsetY - pageStep, contentRect.height(), contentHeight);
    } else if (key == Key::PageDown || key == Key::Space) {
        _viewOffsetY = clampViewOffset(_viewOffsetY + pageStep, contentRect.height(), contentHeight);
    } else if (key == Key::Home) {
        _viewOffsetY = BlockCoordinate{0};
    } else if (key == Key::End) {
        _viewOffsetY = clampViewOffset(contentHeight - contentRect.height(), contentRect.height(), contentHeight);
    } else if (key == U's' || key == U'S') {
        advanceDocumentStylePreset();
    }
}

void HtmlViewerApp::onRenderToBuffer() {
    _buffer.fill(Block{U' ', bg::Black});
    const auto headerRect =
        BlockRectangle{BlockCoordinate{0}, BlockCoordinate{0}, _buffer.size().width(), BlockCoordinate{1}};
    const auto contentRect = contentRectForBuffer(_buffer.size());
    const auto footerRect = BlockRectangle{
        BlockCoordinate{0}, _buffer.size().height() - BlockCoordinate{1}, _buffer.size().width(), BlockCoordinate{1}};
    renderDocumentIfRequired(contentRect.width());
    drawHeader(headerRect);
    drawDocument(contentRect);
    drawFooter(footerRect);
}

void HtmlViewerApp::renderDocumentIfRequired(const BlockCoordinate contentWidth) {
    const auto normalizedWidth = std::max(contentWidth, BlockCoordinate{1});
    if (!_documentDirty && _renderedContentWidth == normalizedWidth) {
        return;
    }
    const auto &documentStyle = TerminalDocumentStyle::defaultStyle(_documentStylePreset);
    const auto baseStyle = documentStyle.baseTextStyle();
    _documentBuffer->resize(BlockSize{normalizedWidth, BlockCoordinate{1}});
    _documentBuffer->setFillChar(Block{U' ', baseStyle});
    _documentBuffer->fill(_documentBuffer->rect(), Block{U' ', baseStyle});
    _documentBuffer->setStyle(baseStyle);
    _documentBuffer->clearScreen();
    _documentBuffer->moveHome();
    TerminalDocumentRenderer{documentStyle}.renderTo(*_documentBuffer, _document);
    _documentView.setContent(_documentBuffer);
    _renderedContentWidth = normalizedWidth;
    _documentDirty = false;
    _viewOffsetY =
        clampViewOffset(_viewOffsetY, contentRectForBuffer(canvasSize()).height(), _documentBuffer->size().height());
}

void HtmlViewerApp::drawHeader(const BlockRectangle rect) {
    _buffer.fill(rect, Block{U' ', bg::Blue});
    _buffer.drawBlockText(
        el::StringFormat{"HTML Viewer  |  {}"_el}.build(displayName()),
        rect.insetBy(BlockMargins{1, 0}),
        Alignment::CenterLeft,
        Color{fg::BrightWhite, bg::Blue});
    _buffer.drawBlockText(
        el::StringFormat{"{}  |  {} x {}"_el}.build(
            documentStylePresetName(), _documentBuffer->size().width(), _documentBuffer->size().height()),
        rect.insetBy(BlockMargins{1, 0}),
        Alignment::CenterRight,
        Color{fg::BrightCyan, bg::Blue});
}

void HtmlViewerApp::drawFooter(const BlockRectangle rect) {
    _buffer.fill(rect, Block{U' ', bg::BrightBlack});
    _buffer.drawBlockText(
        locationText(),
        rect.insetBy(BlockMargins{1, 0}),
        Alignment::CenterLeft,
        Color{fg::BrightGreen, bg::BrightBlack});

    auto help = BlockStringEditor{};
    help.append(
        fg::BrightYellow,
        Key{Key::Up}.toDisplayText(),
        " "_el,
        Key{Key::Down}.toDisplayText(),
        fg::BrightWhite,
        " scroll  "_el,
        fg::BrightYellow,
        Key{Key::PageUp}.toDisplayText(),
        " "_el,
        Key{Key::PageDown}.toDisplayText(),
        fg::BrightWhite,
        " page  "_el,
        fg::BrightYellow,
        "[S]"_el,
        fg::BrightWhite,
        " style  "_el,
        fg::BrightYellow,
        "[Q]"_el,
        " "_el,
        Key{Key::Escape}.toDisplayText(),
        fg::BrightWhite,
        " quit"_el);
    _buffer.drawBlockText(BlockText{help, rect.insetBy(BlockMargins{1, 0}), Alignment::CenterRight});
}

void HtmlViewerApp::drawDocument(const BlockRectangle rect) {
    const auto baseStyle = TerminalDocumentStyle::defaultStyle(_documentStylePreset).baseTextStyle();
    _buffer.fill(rect, Block{U' ', baseStyle});
    updateView(rect.size());
    _buffer.drawBuffer(_documentView, rect);
}

void HtmlViewerApp::updateView(const BlockSize viewSize) noexcept {
    _viewOffsetY = clampViewOffset(
        _viewOffsetY,
        viewSize.height(),
        _documentBuffer == nullptr ? BlockCoordinate{0} : _documentBuffer->size().height());
    _documentView.setViewRect(
        BlockRectangle{
            BlockCoordinate{0}, _viewOffsetY, viewSize.width(), std::max(viewSize.height(), BlockCoordinate{1})});
}

void HtmlViewerApp::advanceDocumentStylePreset() noexcept {
    switch (_documentStylePreset) {
    case DocumentStylePreset::Plain:
        _documentStylePreset = DocumentStylePreset::Simple;
        break;
    case DocumentStylePreset::Simple:
        _documentStylePreset = DocumentStylePreset::Styled;
        break;
    case DocumentStylePreset::Styled:
        _documentStylePreset = DocumentStylePreset::SystemOutput;
        break;
    case DocumentStylePreset::SystemOutput:
        _documentStylePreset = DocumentStylePreset::Plain;
    }
    _documentDirty = true;
}

auto HtmlViewerApp::locationText() const -> el::String {
    const auto totalLines = std::max(_documentBuffer->size().height(), BlockCoordinate{1});
    const auto topLine = std::min(_viewOffsetY + BlockCoordinate{1}, totalLines);
    const auto bottomLine = std::min(_viewOffsetY + _documentView.viewRect().height(), totalLines);
    const auto percent = static_cast<int>((bottomLine.toRawValue() * 100) / totalLines.toRawValue());
    return el::StringFormat{"lines {}-{} / {}  ({}%)"_el}.build(topLine, bottomLine, totalLines, percent);
}

auto HtmlViewerApp::displayName() const -> el::String {
    const auto name = _htmlFilePath.name();
    return name.isEmpty() ? _htmlFilePath.toString() : name;
}

auto HtmlViewerApp::documentStylePresetName() const noexcept -> el::String {
    switch (_documentStylePreset) {
    case DocumentStylePreset::Plain:
        return "plain"_el;
    case DocumentStylePreset::Simple:
        return "simple"_el;
    case DocumentStylePreset::Styled:
        return "styled"_el;
    case DocumentStylePreset::SystemOutput:
        return "system"_el;
    }
    return "styled"_el;
}

auto HtmlViewerApp::parseDocumentStylePreset(const el::String value, DocumentStylePreset &preset) noexcept -> bool {
    if (value == "plain"_el || value == "default"_el) {
        preset = DocumentStylePreset::Plain;
        return true;
    }
    if (value == "simple"_el || value == "compact"_el) {
        preset = DocumentStylePreset::Simple;
        return true;
    }
    if (value == "styled"_el || value == "extended"_el) {
        preset = DocumentStylePreset::Styled;
        return true;
    }
    return false;
}

auto HtmlViewerApp::contentRectForBuffer(const BlockSize bufferSize) noexcept -> BlockRectangle {
    return BlockRectangle{
        BlockCoordinate{0},
        BlockCoordinate{1},
        bufferSize.width(),
        std::max(bufferSize.height() - BlockCoordinate{2}, BlockCoordinate{1})};
}

auto HtmlViewerApp::clampViewOffset(
    const BlockCoordinate offset, const BlockCoordinate viewHeight, const BlockCoordinate height) noexcept
    -> BlockCoordinate {
    return std::clamp(
        offset, BlockCoordinate{0}, std::max(height - std::max(viewHeight, BlockCoordinate{1}), BlockCoordinate{0}));
}

}
