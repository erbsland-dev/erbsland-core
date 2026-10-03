// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Application.hpp>
#include <erbsland/cterm/Terminal.hpp>

namespace demo {

/// Shared terminal configuration for ordinary runs and documentation capture.
class DemoTerminal final {
public: // factories
    /// Enable terminal output using the optional virtual-terminal environment settings.
    /// @param application The demo application that owns the terminal lifecycle.
    static void enable(el::Application &application);
    /// Create a terminal for a demo application's terminal-creation hook.
    /// @param size Initial screen dimensions for ordinary runs.
    /// @param flags Explicit terminal flags, taking precedence over the environment.
    /// @return The initialized terminal.
    [[nodiscard]] static auto create(el::block::Size size, el::cterm::TerminalFlags flags) -> el::cterm::TerminalPtr;

private:
    DemoTerminal(el::block::Size size, el::cterm::TerminalFlags flags);
    /// Parse one positive decimal dimension, bounded to Terminal's maximum size.
    [[nodiscard]] static auto dimension(const el::String &text) -> int;

private:
    el::block::Size _size;           ///< Full screen dimensions.
    el::cterm::TerminalFlags _flags; ///< Selected terminal features.
};

}
