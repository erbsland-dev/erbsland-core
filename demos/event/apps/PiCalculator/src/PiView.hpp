// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DigitHistory.hpp"
#include "DigitViewport.hpp"
#include "PiSession.hpp"

#include <erbsland/cterm/BlockString.hpp>
#include <erbsland/cterm/Buffer.hpp>
#include <erbsland/cterm/Key.hpp>
#include <erbsland/cterm/Terminal.hpp>
#include <erbsland/cterm/UpdateSettings.hpp>
#include <erbsland/String.hpp>

#include <memory>

namespace demo {

/// Browse the calculation's main-loop history without owning the calculation session.
/// The application may release this view while required worker completion remains pending.
/// @notest{Interactive rendering verified in a PTY; viewport and history have separate unit tests.}
class PiView final {
public:
    /// User actions requiring the application's lifecycle policy.
    enum class Action : uint8_t {
        None, ///< Navigation or no key.
        Stop, ///< Stop calculation but retain the view.
        Quit, ///< Release the view and finish the application.
    };

public:
    /// Create an optional view over independently retained main-loop history.
    /// @param terminal The application's terminal. Must not be null.
    /// @param history History read only on main. Must not be null.
    PiView(el::cterm::TerminalPtr terminal, std::shared_ptr<DigitHistory> history);

public: // updates
    /// Associate operation state without retaining the session.
    /// @param session Calculation session, owned by the application.
    void setSession(std::weak_ptr<PiSession> session);
    /// Mark optional presentation work pending; suitable for a weak event binding.
    void onUpdated();
    /// Read at most sixteen already available keys, applying navigation and returning lifecycle actions.
    /// @return Stop, Quit, or None; never waits for input.
    [[nodiscard]] auto pollKeys() -> Action;
    /// Render a changed viewport or terminal size using cterm's buffered screen update.
    void render();

private:
    /// Apply one navigation key or return a lifecycle request to the application.
    [[nodiscard]] auto handleKey(const el::cterm::Key &key) -> Action;
    /// Read operation state through a temporary weak lock on main.
    [[nodiscard]] auto statusText() const -> el::String;
    /// Create the keyboard help text.
    [[nodiscard]] auto keyboardHelp() const -> el::cterm::BlockString;
    /// Colorize a keyboard string.
    [[nodiscard]] static auto colorizeKeys(const el::String &keyboardString) -> el::cterm::BlockString;

private:
    el::cterm::TerminalPtr _terminal;       ///< Terminal used only on main.
    std::shared_ptr<DigitHistory> _history; ///< History outlives optional notifications.
    std::weak_ptr<PiSession> _session;      ///< Operation state without recipient retention.
    DigitViewport _viewport;                ///< Logical position independent of terminal dimensions.
    el::cterm::Buffer _buffer;              ///< Main-loop screen buffer.
    el::cterm::UpdateSettings _settings;    ///< Minimum-size and alternate-screen behavior.
    bool _dirty{true};                      ///< Redraw requested by data, input, or resizing.
};

}
