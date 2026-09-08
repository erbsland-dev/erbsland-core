// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationTerminalData.hpp"

#include "../../../cterm/BlockStyle.hpp"
#include "../../../cterm/Terminal.hpp"
#include "../../../cterm/TerminalDocumentRenderer.hpp"
#include "../../../cterm/TerminalStream.hpp"
#include "../../../stream/StandardStreams.hpp"
#include "../../../stream/TextOutputStream.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/PlainTextRenderer.hpp"
#include "../../../text/TextDocument.hpp"
#include "../../../text/TextNode.hpp"

#include <utility>

namespace erbsland::core::impl {

using namespace text::literals;

void ApplicationTerminalData::enable(cterm::TerminalPtr terminal) {
    if (_isEnabled) {
        return;
    }
    _terminal = std::move(terminal);
    if (_terminal == nullptr) {
        return;
    }
    _isEnabled = true;
    if (_terminal->isInteractive()) {
        const auto [output, error] = cterm::TerminalStream::createStandardStreams(_terminal);
        _streamRedirect = stream::redirectStandardStreams(output, error);
    }
}

auto ApplicationTerminalData::isEnabled() const noexcept -> bool {
    return _isEnabled;
}

auto ApplicationTerminalData::terminal() const noexcept -> const cterm::TerminalPtr & {
    return _terminal;
}

auto ApplicationTerminalData::systemOutputStyle() const noexcept -> const cterm::TerminalDocumentStyle & {
    return _systemOutputStyle;
}

void ApplicationTerminalData::setSystemOutputStyle(cterm::TerminalDocumentStyle style) noexcept {
    _systemOutputStyle = std::move(style);
}

void ApplicationTerminalData::renderSystemOutput(const text::TextDocument &document) {
    if (document.isEmpty()) {
        return;
    }
    if (_isEnabled && _terminal != nullptr && _terminal->isInteractive()) {
        auto renderer = cterm::TerminalDocumentRenderer{_systemOutputStyle};
        renderer.renderTo(*_terminal, document);
        _terminal->setStyle(cterm::BlockStyle::reset());
        _terminal->flush();
        return;
    }
    const auto output = plainSystemOutputStream(document);
    auto renderer = text::PlainTextRenderer{document};
    output->writeLine(renderer.build());
    output->flush();
}

void ApplicationTerminalData::cleanup() noexcept {
    try {
        if (_streamRedirect.isActive()) {
            stream::stdOut()->flush();
            stream::stdErr()->flush();
        }
        if (_isEnabled && _terminal != nullptr) {
            _terminal->restoreScreen();
            _isEnabled = false;
            _terminal = nullptr;
        }
        _streamRedirect.reset();
    } catch (...) {}
}

auto ApplicationTerminalData::plainSystemOutputStream(const text::TextDocument &document)
    -> stream::TextOutputStreamPtr {
    const auto isErrorDocument = document.root()->style() == "error"_el;
    return isErrorDocument ? stream::stdErr() : stream::stdOut();
}

}
