// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConsoleLogWriterOptions.hpp"
#include "LogWriter.hpp"

#include "../cterm/Terminal_fwd.hpp"

namespace erbsland::log {

/// A log writer that directly renders styled paragraphs on a terminal.
/// @tested{ConsoleLogWriterTest}
class ConsoleLogWriter final : public LogWriter {
public:
    /// Create a writer for the given terminal.
    /// @param terminal The terminal receiving complete log paragraphs.
    /// @param options Style and paragraph layout options.
    explicit ConsoleLogWriter(cterm::TerminalPtr terminal, ConsoleLogWriterOptions options = {});

public: // implements LogWriter
    void write(const LogEntryConstPtr &entry, const LogLineConstPtr &line) override;
    void flush() override;

public: // accessors
    /// Access the target terminal.
    [[nodiscard]] auto terminal() const noexcept -> const cterm::TerminalPtr & { return _terminal; }

private:
    cterm::TerminalPtr _terminal;     ///< Terminal receiving complete log paragraphs.
    ConsoleLogWriterOptions _options; ///< Style and paragraph layout settings.
};

}
