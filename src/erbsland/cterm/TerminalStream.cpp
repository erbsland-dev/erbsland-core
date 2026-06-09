// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TerminalStream.hpp"

#include "Terminal.hpp"

#include "../err/StreamError.hpp"

namespace erbsland::cterm {

TerminalStream::TerminalStream(
    TerminalPtr terminal, BlockStyle style, TerminalStreamSynchronizationPtr synchronization) :
    _terminal{std::move(terminal)}, _style{style}, _synchronization{std::move(synchronization)} {
    if (_synchronization == nullptr) {
        _synchronization = createSynchronization();
    }
}

auto TerminalStream::createSynchronization() -> TerminalStreamSynchronizationPtr {
    return std::make_shared<TerminalStreamSynchronization>();
}

auto TerminalStream::create(TerminalPtr terminal, BlockStyle style, TerminalStreamSynchronizationPtr synchronization)
    -> TerminalStreamPtr {
    return std::make_shared<TerminalStream>(std::move(terminal), style, std::move(synchronization));
}

auto TerminalStream::createStandardStreams(TerminalPtr terminal) -> std::pair<TerminalStreamPtr, TerminalStreamPtr> {
    auto errorAttributes = BlockAttributes{};
    errorAttributes.setBold(true);

    auto synchronization = createSynchronization();
    auto outputStream = create(terminal, BlockStyle::reset(), synchronization);
    auto errorStream =
        create(std::move(terminal), BlockStyle{Color{fg::BrightRed, bg::Default}, errorAttributes}, synchronization);
    return {std::move(outputStream), std::move(errorStream)};
}

auto TerminalStream::encoding() const noexcept -> text::StringEncoding {
    return text::StringEncoding::Utf8;
}

auto TerminalStream::effectiveEncoding() const noexcept -> text::StringEncoding {
    return text::StringEncoding::Utf8;
}

auto TerminalStream::isOpen() const noexcept -> bool {
    return _terminal != nullptr;
}

void TerminalStream::flush() {
    const auto terminal = requireTerminal();
    auto lock = std::scoped_lock{_synchronization->_mutex};
    terminal->flush();
}

void TerminalStream::close() {
    flush();
}

void TerminalStream::write(const text::Char character) {
    const auto terminal = requireTerminal();
    auto lock = std::scoped_lock{_synchronization->_mutex};
    terminal->setStyle(_style);
    terminal->write(Block{character});
    resetTerminalStyle(*terminal);
}

void TerminalStream::write(const text::StringView &text) {
    const auto terminal = requireTerminal();
    auto lock = std::scoped_lock{_synchronization->_mutex};
    terminal->setStyle(_style);
    terminal->write(text);
    resetTerminalStyle(*terminal);
}

void TerminalStream::writeLine() {
    const auto terminal = requireTerminal();
    auto lock = std::scoped_lock{_synchronization->_mutex};
    terminal->setStyle(_style);
    terminal->writeLineBreak();
    resetTerminalStyle(*terminal);
}

void TerminalStream::writeLine(const text::StringView &text) {
    const auto terminal = requireTerminal();
    auto lock = std::scoped_lock{_synchronization->_mutex};
    terminal->setStyle(_style);
    terminal->write(text);
    terminal->writeLineBreak();
    resetTerminalStyle(*terminal);
}

auto TerminalStream::requireTerminal() const -> TerminalPtr {
    if (_terminal == nullptr) {
        throw err::StreamError{"The terminal stream has no terminal."};
    }
    return _terminal;
}

void TerminalStream::resetTerminalStyle(Terminal &terminal) {
    terminal.setStyle(BlockStyle::reset());
}

}
