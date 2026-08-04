// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/all.hpp>
#include <erbsland/core/MakeOneNamespace.hpp>
#include <erbsland/cterm/all.hpp>
#include <erbsland/stream/StandardStreams.hpp>

namespace demo {

using namespace el::text::literals;
using namespace el::bgeo;
using namespace el::cterm;

/// The base class for all demo applications
class TerminalApplication : public el::Application {
public:
    using el::Application::Application;

    // defaults
    ~TerminalApplication() override = default;

protected: // implement el::Application
    void initialize() override;
    [[nodiscard]] auto main() -> erbsland::ExitCode override;

public:
    /// Overwrite this method to initialize the `terminal()` or `_updateSettings` before it is initialized.
    /// Does nothing by default.
    virtual void beforeInitialize();
    /// Overwrite this method to initialize application data, *after* the terminal is initialized.
    /// This method also allows an early exit, e.g. based on the command line arguments.
    /// Does nothing by default.
    /// @return 0 on success to continue, or !=0 to stop the application with this exit code,
    ///   or -1 to exit the application early with exit code 0.
    virtual auto beforeMain() -> int;
    /// Overwrite this method to customize the shutdown behavior.
    /// Does nothing by default.
    virtual void onShutdown();
    /// Overwrite this method if you need to prepare something only when the `_buffer` size changed.
    virtual void onResize();
    /// Overwrite this method to implement the render logic to `_buffer`.
    virtual void onRenderToBuffer();
    /// Overwrite this method if you need the measured `updateScreen()` duration for diagnostics.
    /// @param duration The time spent in `Terminal::updateScreen()` for the most recent frame.
    virtual void onAfterUpdateScreen(std::chrono::nanoseconds duration);
    /// Overwrite/extend this method to implement the key handling.
    /// By default, the `q` key will quit the application.
    virtual void onKey(const Key &key);
    /// Get the loop interval in milliseconds.
    [[nodiscard]] virtual auto loopInterval() const noexcept -> std::chrono::milliseconds;

protected:
    UpdateSettings _updateSettings;
    Buffer _buffer;
    bool _quitRequested{false};
    std::size_t _animationCycle{0};
    bool _screenInitialized{false};
};

}
