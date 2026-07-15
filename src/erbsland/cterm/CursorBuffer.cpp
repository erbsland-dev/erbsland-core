// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CursorBuffer.hpp"

#include "Buffer.hpp"

#include "impl/paragraph/Layout.hpp"
#include "impl/paragraph/Printer.hpp"

#include "../err/ParameterError.hpp"

namespace erbsland::cterm {

void CursorBuffer::validateFillChar(const Block &fillChar) {
    if (fillChar.displayWidth() != 1) {
        throw err::ParameterError{"fillChar must be a single-width character.", "fillChar"};
    }
}

auto CursorBuffer::color() const noexcept -> Color {
    return _currentStyle.color();
}

auto CursorBuffer::blockAttributes() const noexcept -> BlockAttributes {
    return _currentStyle.attributes();
}

void CursorBuffer::setColor(Color color) noexcept {
    _currentStyle.setColor(Color::reset().overlayWith(color));
}

void CursorBuffer::setBlockAttributes(const BlockAttributes attributes) noexcept {
    _currentStyle.setAttributes(attributes.withBase(BlockAttributes::reset()));
}

auto CursorBuffer::maximumSize() const noexcept -> bgeo::BlockSize {
    return _maximumSize;
}

void CursorBuffer::setMaximumSize(bgeo::BlockSize maximumSize) noexcept {
    _maximumSize = maximumSize;
}

auto CursorBuffer::overflowMode() const noexcept -> OverflowMode {
    return _overflowMode;
}

void CursorBuffer::setOverflowMode(const OverflowMode mode) noexcept {
    _overflowMode = mode;
}

auto CursorBuffer::fillChar() const noexcept -> const Block & {
    return _fillChar;
}

void CursorBuffer::setFillChar(const Block fillChar) {
    validateFillChar(fillChar);
    _fillChar = fillChar;
}

void CursorBuffer::setForeground(Foreground color) noexcept {
    if (color == Foreground::Inherited) {
        color = Foreground::Default;
    }
    _currentStyle.setFg(color);
}

void CursorBuffer::setBackground(Background color) noexcept {
    if (color == Background::Inherited) {
        color = Background::Default;
    }
    _currentStyle.setBg(color);
}

auto CursorBuffer::supportedBlockAttributes() const noexcept -> BlockAttributes {
    return BlockAttributes::all();
}

void CursorBuffer::moveCursor(const bgeo::BlockPosition posOrDelta, const MoveMode mode) noexcept {
    if (std::abs(posOrDelta.x().toRawValue()) > cMaximumSize.width().toRawValue() ||
        std::abs(posOrDelta.y().toRawValue()) > cMaximumSize.height().toRawValue()) {
        return; // ignore calls with extreme values.
    }
    if (posOrDelta == bgeo::BlockPosition{0, 0}) {
        return;
    }
    _wrapOnNextChar = false;
    if (mode == MoveMode::Absolute) {
        _cursorPosition = rect().clamp(posOrDelta);
    } else {
        _cursorPosition = rect().clamp(_cursorPosition + posOrDelta);
    }
}

void CursorBuffer::setAutoWrap(const bool enabled) noexcept {
    _autoWrap = enabled;
}

void CursorBuffer::clearScreen() noexcept {
    fill(_fillChar);
    _cursorPosition = bgeo::BlockPosition{};
    _wrapOnNextChar = false;
}

void CursorBuffer::writeResolvedBlock(const Block &character) noexcept {
    const auto displayWidth = character.displayWidth();
    if (displayWidth == 0 || displayWidth > 2) {
        return;
    }
    auto x = _cursorPosition.x();
    auto y = _cursorPosition.y();
    auto width = _size.width();
    if (_wrapOnNextChar || x >= width) {
        if (_autoWrap) {
            writeLineBreak();
        } else {
            _cursorPosition = bgeo::BlockPosition{width - 1, y};
        }
        _wrapOnNextChar = false;
        x = _cursorPosition.x();
        y = _cursorPosition.y();
        width = _size.width();
    }
    const auto lastColumn = width - 1;
    if (x == lastColumn) {
        if (displayWidth == 2) {
            if (!_autoWrap) {
                return;
            }
            writeLineBreak();
            x = _cursorPosition.x();
            y = _cursorPosition.y();
            set(bgeo::BlockPosition{x, y}, character);
            _cursorPosition = bgeo::BlockPosition{x + displayWidth, y};
        } else {
            set(bgeo::BlockPosition{x, y}, character);
            _wrapOnNextChar = _autoWrap;
        }
    } else {
        set(bgeo::BlockPosition{x, y}, character);
        x += displayWidth;
        if (x >= width) {
            x = width - 1;
            _wrapOnNextChar = _autoWrap;
        }
        _cursorPosition = bgeo::BlockPosition{x, y};
    }
}

void CursorBuffer::write(const Block &character) noexcept {
    writeResolvedBlock(character.withBase(_currentStyle));
}

void CursorBuffer::write(const BlockStringView &str) noexcept {
    for (const auto &character : str) {
        writeResolvedBlock(character.withBase(_currentStyle));
    }
}

void CursorBuffer::writeResolved(const Block &character) noexcept {
    writeResolvedBlock(character);
}

void CursorBuffer::writeResolved(const BlockStringView &str) noexcept {
    for (const auto &character : str) {
        writeResolvedBlock(character);
    }
}

void CursorBuffer::write(const ReadableBuffer &buffer) noexcept {
    for (auto y = bgeo::BlockCoordinate{0}; y < buffer.size().height(); ++y) {
        for (auto x = bgeo::BlockCoordinate{0}; x < buffer.size().width(); ++x) {
            write(buffer.get(bgeo::BlockPosition{x, y}));
        }
        writeLineBreak();
    }
}

void CursorBuffer::writeLineBreak() noexcept {
    _wrapOnNextChar = false;
    if (_cursorPosition.y() < _size.height() - 1) {
        _cursorPosition = bgeo::BlockPosition{bgeo::BlockCoordinate{0}, _cursorPosition.y() + 1};
    } else {
        switch (_overflowMode) {
        case OverflowMode::Wrap:
            _cursorPosition = bgeo::BlockPosition{0, 0};
            break;
        case OverflowMode::Shift:
            shift(bgeo::BlockDirection::North, _fillChar, 1);
            _cursorPosition = bgeo::BlockPosition{bgeo::BlockCoordinate{0}, _size.height() - 1};
            break;
        case OverflowMode::ExpandThenShift:
            if (_size.height() < _maximumSize.height()) {
                resize(_size + bgeo::BlockSize{0, 1}, BufferResizeMode::PreserveContent, _fillChar);
            } else {
                shift(bgeo::BlockDirection::North, _fillChar, 1);
            }
            _cursorPosition = bgeo::BlockPosition{bgeo::BlockCoordinate{0}, _size.height() - 1};
            break;
        case OverflowMode::ExpandThenWrap:
            if (_size.height() < _maximumSize.height()) {
                resize(_size + bgeo::BlockSize{0, 1}, BufferResizeMode::PreserveContent, _fillChar);
                _cursorPosition = bgeo::BlockPosition{bgeo::BlockCoordinate{0}, _size.height() - 1};
            } else {
                _cursorPosition = bgeo::BlockPosition{0, 0};
            }
        }
    }
}

auto CursorBuffer::printParagraphImpl(const BlockStringView &paragraph, const ParagraphOptions &options) noexcept
    -> int {
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
        if (options.onError() == ParagraphOnError::Empty) {
            return 0;
        }
        write(paragraph);
        auto lineCount = std::max(paragraph.terminalLines(std::max(width.toRawValue(), 1)), 1);
        writeLineBreak();
        if (options.paragraphSpacing() == ParagraphSpacing::DoubleLine) {
            writeLineBreak();
            lineCount += 1;
        }
        return lineCount;
    }
    if (layout.empty()) {
        writeLineBreak();
        auto lineCount = 1;
        if (options.paragraphSpacing() == ParagraphSpacing::DoubleLine) {
            writeLineBreak();
            lineCount += 1;
        }
        return lineCount;
    }
    const auto lineCount =
        impl::paragraph::Printer{
            *this,
            x1.toRawValue(),
            width.toRawValue(),
            options.alignment(),
            layout,
            paragraph,
            options,
            options.backgroundMode()}
            .print();
    if (options.paragraphSpacing() == ParagraphSpacing::DoubleLine) {
        writeLineBreak();
        return lineCount + 1;
    }
    return lineCount;
}

}
