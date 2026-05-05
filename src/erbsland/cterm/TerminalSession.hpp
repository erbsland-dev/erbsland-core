// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Terminal.hpp"

namespace erbsland::cterm {

/// A scoped terminal session.
/// Automatically calls `initializeScreen` on construction and `restoreScreen` on destruction.
class TerminalSession {
public:
    /// Create the session and call `initializeScreen`.
    explicit TerminalSession(Terminal &terminal) : _terminal(terminal) { _terminal.initializeScreen(); }
    /// Terminate the session and call `restoreScreen`.
    ~TerminalSession() { _terminal.restoreScreen(); };

    // defaults
    TerminalSession(const TerminalSession &) = delete;
    TerminalSession(TerminalSession &&) = delete;
    auto operator=(const TerminalSession &) -> TerminalSession & = delete;
    auto operator=(TerminalSession &&) -> TerminalSession & = delete;

public:
    /// Access the terminal instance.
    [[nodiscard]] auto terminal() const noexcept -> Terminal & { return _terminal; }

private:
    Terminal &_terminal;
};

}
