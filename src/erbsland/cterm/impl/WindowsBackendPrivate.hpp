// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "WindowsBackendPrivate_fwd.hpp"
#include "WindowsSignalDispatcher.hpp"

#include "../Input.hpp"
#include "../TerminalFlags.hpp"

#include "../../core/impl/WindowsApi.hpp"
#include "../../text/CombinedChar.hpp"

#include <signal.h>

#include <csignal>
#include <cstdint>
#include <deque>
#include <memory>
#include <optional>

namespace erbsland::cterm::impl {

/// The private implementation of the WindowsBackend.
class WindowsBackendPrivate {
public:
    /// Create private Windows backend state.
    /// @param flags The requested terminal features.
    explicit WindowsBackendPrivate(const TerminalFlags flags) : _terminalFlags{flags} {}

public:
    TerminalFlags _terminalFlags;                            ///< The terminal flags.
    bool _initialized{false};                                ///< If the platform was initialized.
    bool _isInteractive{true};                               ///< If the backend is interactive.
    bool _cursorStateSaved{false};                           ///< If the backend saved the cursor state.
    bool _cursorVisible{true};                               ///< The current cursor visibility state.
    Input::Mode _inputMode{Input::Mode::ReadLine};           ///< The current input mode.
    bool _isAlternateScreenActive{false};                    ///< Flag if the alternate screen is active.
    std::deque<Key> _pendingKeys;                            ///< Queued decoded key events.
    std::optional<text::CombinedChar> _pendingTextInput;     ///< Buffered translated Unicode text input.
    std::optional<char16_t> _pendingHighSurrogate;           ///< Stored first UTF-16 surrogate for the next event.
    std::unique_ptr<WindowsSignalDispatcher> _signalHandler; ///< Helper that forwards termination events safely.
    HANDLE outputHandle{INVALID_HANDLE_VALUE};               ///< The output handle.
};

}
