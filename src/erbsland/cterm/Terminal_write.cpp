// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Terminal.hpp"

#include "Buffer.hpp"
#include "BufferView.hpp"

#include "impl/AnsiSequence.hpp"
#include "impl/BlockPrintContextToTerminal.hpp"
#include "impl/paragraph/Layout.hpp"
#include "impl/paragraph/Printer.hpp"

#include "../err/RuntimeError.hpp"
#include "../text/Literals.hpp"
#include "../text/StringEditor.hpp"

#include <algorithm>
#include <cassert>
#include <vector>

namespace erbsland::cterm {

using namespace text::literals;

void Terminal::write(const Block &character) noexcept {
    const auto resolvedCharacter = character.withBase(_style);
    setStyle(resolvedCharacter.style());
    _lineBuffer.write(resolvedCharacter);
    _lineBuffer.handleEmit();
}

void Terminal::write(const BlockString &str) noexcept {
    for (const auto &character : str) {
        const auto resolvedCharacter = character.withBase(_style);
        setStyle(resolvedCharacter.style());
        _lineBuffer.write(resolvedCharacter);
    }
    _lineBuffer.handleEmit();
}

void Terminal::writeResolved(const Block &character) noexcept {
    setStyle(character.style());
    _lineBuffer.write(character);
    _lineBuffer.handleEmit();
}

void Terminal::writeResolved(const BlockString &str) noexcept {
    for (const auto &character : str) {
        setStyle(character.style());
        _lineBuffer.write(character);
    }
    _lineBuffer.handleEmit();
}

void Terminal::write(const ReadableBuffer &buffer) noexcept {
    {
        impl::LineBuffer::EmitLockGuard emitLock{_lineBuffer};
        writeImpl(buffer, false);
    }
    _lineBuffer.handleEmit();
}

void Terminal::writeLineBreak() noexcept {
    _lineBuffer.write("\n"_el);
    _lineBuffer.handleEmit();
}

auto Terminal::createPrintContext() noexcept -> BlockPrintContextPtr {
    return std::make_unique<impl::BlockPrintContextToTerminal>(*this);
}

auto Terminal::printParagraphImpl(const BlockString &paragraph, const ParagraphOptions &options) noexcept -> int {
    const auto margins = options.margins();
    const auto x1 = std::max(margins.left(), bgeo::BlockCoordinate{0});
    const auto width = std::max(
        size().width() - std::max(margins.left(), bgeo::BlockCoordinate{0}) -
            std::max(margins.right(), bgeo::BlockCoordinate{0}),
        bgeo::BlockCoordinate{0});
    const auto layout =
        impl::paragraph::Layout{
            paragraph, width.toRawValue(), options, impl::paragraph::LayoutNewlineMode::HardLineBreak}
            .build();
    if (!layout.valid()) {
        return printParagraphPlainOutput(paragraph, options);
    }
    if (layout.empty()) {
        return finishParagraphWithExplicitLineBreaks(0, options.paragraphSpacing());
    }
    const auto lineCount = [&]() -> int {
        impl::LineBuffer::EmitLockGuard emitLock{_lineBuffer};
        return impl::paragraph::Printer{
            *this,
            x1.toRawValue(),
            width.toRawValue(),
            options.alignment(),
            layout,
            paragraph,
            options,
            options.backgroundMode()}
            .print();
    }();
    _lineBuffer.handleEmit();
    if (options.paragraphSpacing() == ParagraphSpacing::DoubleLine) {
        writeLineBreak();
        return lineCount + 1;
    }
    return lineCount;
}

auto Terminal::printParagraphPlainOutput(const BlockString &paragraph, const ParagraphOptions &options) noexcept
    -> int {
    if (options.onError() == ParagraphOnError::Empty) {
        return 0;
    }
    write(paragraph);
    const auto margins = options.margins();
    const auto width = std::max(
        size().width() - std::max(margins.left(), bgeo::BlockCoordinate{0}) -
            std::max(margins.right(), bgeo::BlockCoordinate{0}),
        bgeo::BlockCoordinate{1});
    return finishParagraphWithExplicitLineBreaks(
        paragraph.terminalLines(width.toRawValue()), options.paragraphSpacing());
}

auto Terminal::finishParagraphWithExplicitLineBreaks(
    const int renderedLines, const ParagraphSpacing paragraphSpacing) noexcept -> int {
    writeLineBreak();
    auto totalLines = std::max(renderedLines, 1);
    if (paragraphSpacing == ParagraphSpacing::DoubleLine) {
        writeLineBreak();
        totalLines += 1;
    }
    return totalLines;
}

}
