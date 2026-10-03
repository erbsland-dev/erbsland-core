// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PiCalculatorApp.hpp"

#include <DemoTerminal.hpp>
#include <erbsland/ApplicationError.hpp>
#include <erbsland/BindEvent.hpp>
#include <erbsland/CpCount.hpp>
#include <erbsland/CpIndex.hpp>
#include <erbsland/cterm/Input.hpp>
#include <erbsland/CurrentEvents.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/OptionError.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionType.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/TimeAmounts.hpp>

#include <exception>
#include <utility>

namespace demo {

using namespace el::text::literals;

void PiCalculatorApp::initialize() {
    DemoTerminal::enable(*this);
    info().setApplicationName("Pi Calculator"_el);
}

void PiCalculatorApp::registerCommandLineOptions(const el::OptionsPtr &options) {
    options->setHelpDescription("Calculate exact decimal digits of pi while browsing a live terminal display."_el);
    options->addOption({"--digits"_el, "digits"_el})
        .setType(el::OptionType::Integer)
        .setHelp("Calculate this many fractional digits and exit; omitted means unlimited."_el);
    options->addOption({"--timeout"_el, "timeout"_el})
        .setType(el::OptionType::Integer)
        .setDefaultValue(0)
        .setHelp("Stop and exit after this many seconds; zero means unlimited."_el);
    options->addOption({"--plain"_el, "plain"_el})
        .setHelp(
            "Stream digits without screen controls. Interactive keys: S stop, Q/Esc quit, F follow, arrows scroll."_el);
}

void PiCalculatorApp::handleCommandLineOptions(const el::OptionValuesPtr &values) {
    const auto timeout = values->getInteger("timeout"_el);
    if (timeout < 0) {
        throw el::OptionError{
            "The timeout must be non-negative."_el,
            values->value("timeout"_el),
        };
    }
    _timeout = el::Seconds{timeout};
    if (values->value("digits"_el)) {
        const auto digits = values->getInteger("digits"_el);
        if (!std::in_range<std::size_t>(digits)) {
            throw el::OptionError{
                "The fractional digit count must be a non-negative size."_el,
                values->value("digits"_el),
            };
        }
        _limit = el::CpCount::fromSizeTOrThrow(static_cast<std::size_t>(digits));
    }
    _plain = values->getFlag("plain"_el) || !terminal()->isInteractive();

    // Start the application as soon the main loop has started.
    events()->invoke([this]() -> void { begin(); });
}

void PiCalculatorApp::begin() {
    _history = std::make_shared<DigitHistory>();
    PiSession::OnUpdated updated;
    if (_plain) {
        terminal()->print("3."_el);
        updated = [this]() -> void { printNewDigits(); };
    } else {
        terminal()->input().setMode(el::cterm::Input::Mode::Key);
        _view = std::make_shared<PiView>(terminal(), _history);
        // Optional presentation must not keep the view alive after Q or timeout releases it.
        updated = el::bindWeakEvent(std::weak_ptr<PiView>{_view}, &PiView::onUpdated);
    }
    _session = std::make_shared<PiSession>(
        el::currentEvents(), _history, std::move(updated), [this](std::exception_ptr error) -> void {
            onCalculationFinished(std::move(error));
        });
    if (_view) {
        _view->setSession(_session);
    }
    _session->start(_limit);
    _refresh = events()->createTimer([this]() -> void { tick(); });
    _refresh->startFixedDelay(el::Milliseconds{33});
    if (_timeout.isPositive()) {
        _deadline = events()->createTimer([this]() -> void { requestExit(); });
        _deadline->startOnce(_timeout);
    }
}

void PiCalculatorApp::tick() {
    if (!_view) {
        return;
    }
    const auto action = _view->pollKeys();
    if (action == PiView::Action::Quit) {
        requestExit();
        return;
    }
    if (action == PiView::Action::Stop) {
        _session->requestStop();
    }
    _view->render();
}

void PiCalculatorApp::printNewDigits() {
    const auto count = _history->count();
    terminal()->print(_history->range({el::CpIndex::end(_printed), el::CpCount::infinite()}));
    terminal()->flush();
    _printed = count;
}

void PiCalculatorApp::requestExit() {
    _exitRequested = true;
    // Release optional UI immediately, but keep main open for strongly bound required completion.
    _view.reset();
    if (_session->isFinished()) {
        quit();
    } else {
        _session->requestStop();
    }
}

void PiCalculatorApp::onCalculationFinished(std::exception_ptr error) {
    if (error) {
        std::rethrow_exception(error);
    }
    if (_plain) {
        printNewDigits();
        terminal()->printLine();
    }
    // S leaves the stopped view available even with --digits. Only normal finite completion exits automatically.
    const auto finiteCompletion = _limit.has_value() && !_session->isStopping();
    if (_exitRequested || finiteCompletion || _plain) {
        quit();
    }
}

void PiCalculatorApp::cleanup() noexcept {
    _view.reset();
    if (_refresh) {
        _refresh->stop();
        _refresh.reset();
    }
    if (_deadline) {
        _deadline->stop();
        _deadline.reset();
    }
    if (_session) {
        _session->shutdown();
        _session.reset();
    }
}

}
