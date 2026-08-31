// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ConsoleLogWriter.hpp"

#include "../cterm/BlockStringEditor.hpp"
#include "../cterm/Terminal.hpp"
#include "../err/ParameterError.hpp"
#include "../text/Literals.hpp"

#include <utility>

namespace erbsland::log {

using namespace text::literals;

ConsoleLogWriter::ConsoleLogWriter(cterm::TerminalPtr terminal, ConsoleLogWriterOptions options) :
    _terminal{std::move(terminal)}, _options{std::move(options)} {
    if (!_terminal) {
        throw err::ParameterError{"A console log writer requires a terminal."_el, "terminal"_el};
    }
    if (!_terminal->isInteractive()) {
        _terminal->setOutputMode(cterm::Terminal::OutputMode::BlockText);
    }
}

void ConsoleLogWriter::write(const LogEntryConstPtr &entry, const LogLineConstPtr &line) {
    auto rendered = cterm::BlockStringEditor{};
    for (const auto &segment : line->segments()) {
        rendered.append(_options.partStyle(segment.part, entry->level()), segment.text);
    }
    auto guard = _terminal->synchronizeOutput();
    _terminal->printParagraph(rendered, _options.paragraphOptions());
    _terminal->flush();
}

void ConsoleLogWriter::flush() {
    auto guard = _terminal->synchronizeOutput();
    _terminal->flush();
}

}
