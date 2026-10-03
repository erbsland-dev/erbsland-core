// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DigitHistory.hpp"
#include "PiSession.hpp"
#include "PiView.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/CpCount.hpp>
#include <erbsland/cterm/TerminalSession.hpp>
#include <erbsland/EventTimer.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/TimeDelta.hpp>

#include <cstddef>
#include <memory>
#include <optional>

namespace demo {

/// Run a cancellable pi calculation while main handles keyboard input, scrolling, and timeout.
/// Required callbacks retain the session; optional presentation uses a weak event binding.
/// @notest{Application scenarios and terminal restoration verified through command-line and PTY checks.}
class PiCalculatorApp final : public el::Application {
public:
    using Application::Application;

protected: // implement Application
    void initialize() override;
    void registerCommandLineOptions(const el::OptionsPtr &options) override;
    void handleCommandLineOptions(const el::OptionValuesPtr &values) override;
    void cleanup() noexcept override;

private:
    /// Construct operation state and bind delivery while main is the current loop.
    void begin();
    /// Poll input without waiting and redraw from main-loop state.
    void tick();
    /// Stream the accepted prefix not yet written in plain mode.
    void printNewDigits();
    /// Release optional presentation and wait for required cancellation completion.
    void requestExit();
    /// Exit after joining, or retain the stopped view for browsing.
    void onCalculationFinished(std::exception_ptr error);

private:
    std::shared_ptr<DigitHistory> _history; ///< Required data, independent of presentation.
    std::shared_ptr<PiSession> _session;    ///< Retained until the calculation thread has joined.
    std::shared_ptr<PiView> _view;          ///< Optional recipient, released when the user quits.
    el::EventTimerPtr _refresh;             ///< Main-loop input and redraw timer.
    el::EventTimerPtr _deadline;            ///< Exit deadline, independent of stop-and-browse.
    std::optional<el::CpCount> _limit;      ///< Optional finite fractional-digit count.
    el::TimeDelta _timeout;                 ///< Zero means no application deadline.
    el::CpCount _printed{0};                ///< Plain-output prefix already written.
    bool _plain{false};                     ///< Stream digits without full-screen controls.
    bool _exitRequested{false};             ///< Exit waits for required completion.
};

}
