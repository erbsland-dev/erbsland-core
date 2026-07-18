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
#include "../text/String.hpp"
#include "../text/StringEditor.hpp"

#include <algorithm>
#include <cassert>
#include <vector>

namespace erbsland::cterm {

using namespace text::literals;

using bgeo::BlockCoordinate;
using bgeo::BlockPosition;

auto Terminal::color() const noexcept -> Color {
    return _style.color();
}

auto Terminal::blockAttributes() const noexcept -> BlockAttributes {
    return _style.attributes();
}

auto Terminal::supportedBlockAttributes() const noexcept -> BlockAttributes {
    return BlockAttributes::fromMask(_backend->supportedBlockAttributes().mask());
}

void Terminal::setForeground(Foreground color) noexcept {
    if (_outputMode == OutputMode::BlockText) {
        return;
    }
    if (color == Foreground::Inherited) {
        color = Foreground::Default;
    }
    if (color != _style.fg()) {
        _style.setFg(color);
        if (_backend->supportsColorCodes()) {
            _lineBuffer.write(impl::ansi_sequence::color(color.ansiCode()));
            _lineBuffer.handleEmit();
        } else {
            _backend->emitColor(_style.color());
        }
    }
}

void Terminal::setBlockAttributes(BlockAttributes attributes) noexcept {
    if (_outputMode == OutputMode::BlockText) {
        return;
    }
    attributes = normalizedSupportedAttributes(attributes);
    if (attributes == _style.attributes()) {
        return;
    }
    const auto supportedCodeMask = static_cast<uint8_t>(
        _backend->supportedBlockAttributes().mask() & _backend->supportedBlockAttributeCodes().mask());
    const auto previousCodeAttributes =
        BlockAttributes::fromMasks(_style.attributes().enabledMask() & supportedCodeMask, supportedCodeMask);
    const auto newCodeAttributes =
        BlockAttributes::fromMasks(attributes.enabledMask() & supportedCodeMask, supportedCodeMask);
    if (previousCodeAttributes != newCodeAttributes) {
        emitCharAttributeCodes(previousCodeAttributes, newCodeAttributes);
    }
    const auto callbackMask = static_cast<uint8_t>(_backend->supportedBlockAttributes().mask() & ~supportedCodeMask);
    if (((_style.attributes().enabledMask() ^ attributes.enabledMask()) & callbackMask) != 0) {
        flush();
        _backend->emitBlockAttributes(
            BlockAttributes::fromMasks(attributes.enabledMask() & callbackMask, callbackMask));
    }
    _style.setAttributes(attributes);
}

void Terminal::setBackground(Background color) noexcept {
    if (_outputMode == OutputMode::BlockText) {
        return;
    }
    if (color == Background::Inherited) {
        color = Background::Default;
    }
    if (color != _style.bg()) {
        _style.setBg(color);
        if (_backend->supportsColorCodes()) {
            _lineBuffer.write(impl::ansi_sequence::color(color.ansiCode()));
            _lineBuffer.handleEmit();
        } else {
            _backend->emitColor(_style.color());
        }
    }
}

void Terminal::setColor(Color color) noexcept {
    if (_outputMode == OutputMode::BlockText) {
        return;
    }
    if (color.fg() == Foreground::Inherited) {
        color.setFg(Foreground::Default);
    }
    if (color.bg() == Background::Inherited) {
        color.setBg(Background::Default);
    }
    if (color == _style.color()) {
        return;
    }
    if (_backend->supportsColorCodes()) {
        if (color.fg() != _style.fg() && color.bg() != _style.bg()) {
            _lineBuffer.write(impl::ansi_sequence::color(color.fg().ansiCode(), color.bg().ansiCode()));
        } else if (color.fg() != _style.fg()) {
            _lineBuffer.write(impl::ansi_sequence::color(color.fg().ansiCode()));
        } else if (color.bg() != _style.bg()) {
            _lineBuffer.write(impl::ansi_sequence::color(color.bg().ansiCode()));
        }
        _lineBuffer.handleEmit();
    } else {
        _backend->emitColor(color);
    }
    _style.setColor(color);
}

auto Terminal::canUseLineBuffer() const noexcept -> bool {
    return _backend->supportsColorCodes() && _backend->supportsCursorCodes() &&
        _backend->supportedBlockAttributes().mask() == _backend->supportedBlockAttributeCodes().mask();
}

auto Terminal::normalizedSupportedAttributes(BlockAttributes attributes) const noexcept -> BlockAttributes {
    const auto supportedMask = _backend->supportedBlockAttributes().mask();
    attributes = attributes.withBase(BlockAttributes::reset());
    return BlockAttributes::fromMasks(attributes.enabledMask() & supportedMask, supportedMask)
        .withBase(BlockAttributes::reset());
}

void Terminal::emitCharAttributeCodes(
    const BlockAttributes previousAttributes, const BlockAttributes newAttributes) noexcept {
    auto codes = std::vector<int>{};
    if (previousAttributes.isBold() != newAttributes.isBold() || previousAttributes.isDim() != newAttributes.isDim()) {
        if (!newAttributes.isBold() && !newAttributes.isDim()) {
            codes.push_back(22);
        } else {
            if (previousAttributes.isBold() || previousAttributes.isDim()) {
                codes.push_back(22);
            }
            if (newAttributes.isBold()) {
                codes.push_back(1);
            }
            if (newAttributes.isDim()) {
                codes.push_back(2);
            }
        }
    }
    if (previousAttributes.isItalic() != newAttributes.isItalic()) {
        codes.push_back(newAttributes.isItalic() ? 3 : 23);
    }
    if (previousAttributes.isUnderline() != newAttributes.isUnderline()) {
        codes.push_back(newAttributes.isUnderline() ? 4 : 24);
    }
    if (previousAttributes.isBlink() != newAttributes.isBlink()) {
        codes.push_back(newAttributes.isBlink() ? 5 : 25);
    }
    if (previousAttributes.isReverse() != newAttributes.isReverse()) {
        codes.push_back(newAttributes.isReverse() ? 7 : 27);
    }
    if (previousAttributes.isHidden() != newAttributes.isHidden()) {
        codes.push_back(newAttributes.isHidden() ? 8 : 28);
    }
    if (previousAttributes.isStrikethrough() != newAttributes.isStrikethrough()) {
        codes.push_back(newAttributes.isStrikethrough() ? 9 : 29);
    }
    if (codes.empty()) {
        return;
    }
    auto sequence = text::StringEditor{"\x1b["_el};
    auto firstCode = true;
    for (const auto code : codes) {
        if (!firstCode) {
            sequence.append(U';');
        }
        sequence.append(text::String::fromInteger(code));
        firstCode = false;
    }
    sequence.append(U'm');
    _lineBuffer.write(sequence);
    _lineBuffer.handleEmit();
}

void Terminal::moveLeft(const BlockCoordinate count) noexcept {
    if (_outputMode == OutputMode::BlockText) {
        return;
    }
    if (!_backend->supportsCursorCodes()) {
        _backend->moveCursor(BlockPosition{-count, BlockCoordinate{0}}, MoveMode::Relative);
        return;
    }
    _lineBuffer.write(impl::ansi_sequence::moveLeft(count.toRawValue()));
    _lineBuffer.handleEmit();
}

void Terminal::moveRight(const BlockCoordinate count) noexcept {
    if (_outputMode == OutputMode::BlockText) {
        return;
    }
    if (!_backend->supportsCursorCodes()) {
        _backend->moveCursor(BlockPosition{count, BlockCoordinate{0}}, MoveMode::Relative);
        return;
    }
    _lineBuffer.write(impl::ansi_sequence::moveRight(count.toRawValue()));
    _lineBuffer.handleEmit();
}

void Terminal::moveUp(const BlockCoordinate count) noexcept {
    if (_outputMode == OutputMode::BlockText) {
        return;
    }
    if (!_backend->supportsCursorCodes()) {
        _backend->moveCursor(BlockPosition{BlockCoordinate{0}, -count}, MoveMode::Relative);
        return;
    }
    _lineBuffer.write(impl::ansi_sequence::moveUp(count.toRawValue()));
    _lineBuffer.handleEmit();
}

void Terminal::moveDown(const BlockCoordinate count) noexcept {
    if (_outputMode == OutputMode::BlockText) {
        return;
    }
    if (!_backend->supportsCursorCodes()) {
        _backend->moveCursor(BlockPosition{BlockCoordinate{0}, count}, MoveMode::Relative);
        return;
    }
    _lineBuffer.write(impl::ansi_sequence::moveDown(count.toRawValue()));
    _lineBuffer.handleEmit();
}

void Terminal::flush() noexcept {
    _lineBuffer.handleEmit(true); // force-emit the line buffer.
    _backend->emitFlush();        // flush the output to the terminal.
}

}
