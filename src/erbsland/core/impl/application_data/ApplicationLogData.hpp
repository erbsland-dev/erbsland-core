// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationLogData_fwd.hpp"
#include "ApplicationTerminalData_fwd.hpp"

#include "../../../cterm/Terminal_fwd.hpp"
#include "../../../i18n/DisplayTextMap_fwd.hpp"
#include "../../../log/impl/ConsoleLogWriter_fwd.hpp"
#include "../../../log/impl/LastErrorsLogWriter_fwd.hpp"
#include "../../../log/LogManager_fwd.hpp"
#include "../../../unit/ExitCode.hpp"
#include "../../LastErrorDumpMode.hpp"

#include <mutex>

namespace erbsland::core::impl {

/// Application logging state, default routing, and retained-error cleanup.
/// @tested{ApplicationLogTest}
class ApplicationLogData final {
public:
    /// Access the log manager, creating its default terminal route on first use.
    [[nodiscard]] auto manager(const cterm::TerminalPtr &terminal) -> log::LogManager &;
    /// Enable retained-error output and update its display policy.
    void enableLastErrorDump(log::LogManager &manager, LastErrorDumpMode mode);
    /// Shut down logging and render a retained-error dump when requested.
    void cleanup(
        unit::ExitCode exitCode,
        const i18n::DisplayTextMapConstPtr &displayText,
        ApplicationTerminalData *terminalData) noexcept;

private:
    std::mutex _mutex;                                         ///< Serializes logging state.
    log::LogManagerPtr _manager;                               ///< Application log manager.
    log::impl::ConsoleLogWriterPtr _consoleWriter;             ///< Default console writer.
    log::impl::LastErrorsLogWriterPtr _lastErrorsWriter;       ///< Optional retained-error writer.
    LastErrorDumpMode _dumpMode{LastErrorDumpMode::OnFailure}; ///< Retained-error display policy.
};

}
