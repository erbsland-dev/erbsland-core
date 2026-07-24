// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ReadLineApp.hpp"

namespace demo {

using namespace el::text::literals;

void ReadLineApp::initialize() {
    info().setApplicationName("ReadLine Demo"_el);
    info().setApplicationVersion(el::Version{0, 1, 0});
    info().setAuthorName("Erbsland DEV"_el);
    info().setLicenseText("Apache-2.0"_el);
    enableTerminal();
}

auto ReadLineApp::main() -> el::ExitCode {
    auto settings = el::cterm::ReadLineOptions{};
    applyConfiguration(settings);
    applyCommandLine(settings);
    if (hasCommandLineValue("dump-config"_el)) {
        dumpConfiguration(settings);
        return el::ExitCode::success();
    }
    if (terminal() == nullptr || !terminal()->isInteractive()) {
        throw el::ApplicationError{el::core::ApplicationErrorContext{
            "Interactive terminal required"_el,
            "The ReadLine demo can only run in an interactive terminal. Use --help to inspect its options."_el}};
    }
    startReadLine(std::move(settings));
    return runEventLoop();
}

void ReadLineApp::cleanup() noexcept {
    if (_updateTimer != nullptr) {
        _updateTimer->stop();
    }
    if (_readLine != nullptr) {
        _readLine->stop();
    }
}

void ReadLineApp::startReadLine(el::cterm::ReadLineOptions settings) {
    writeClock(true);
    _readLine = el::cterm::ReadLine::create(terminal(), std::move(settings));
    _readLine->start();
    _updateTimer = events()->createTimer([this]() -> void { updateReadLine(); });
    _updateTimer->startFixedRate(el::Milliseconds{25});
}

void ReadLineApp::updateReadLine() {
    const auto result = _readLine->update();
    if (result.isIdle()) {
        writeClock(false);
        return;
    }
    finishReadLine(result);
}

void ReadLineApp::writeClock(const bool initial) {
    const auto clock = el::time::DateTime::now()
                           .toTimeZone(el::time::TimeZone::local())
                           .time()
                           .toIsoString(el::time::cDefaultTimeFormat, el::time::DateTimePrecision::Second);
    if (!initial && clock == _lastClockText) {
        return;
    }
    _lastClockText = clock;

    const auto width = std::max(terminal()->size().width(), el::BlockCoordinate{1});
    auto buffer = el::cterm::Buffer{
        el::BlockSize{width, el::BlockCoordinate{1}}, el::cterm::Block{U' ', el::cterm::BlockStyle::reset()}};
    buffer.drawBlockText(
        el::String::fromJoined({"Event loop clock: "_el, clock}),
        buffer.rect().insetBy(el::BlockMargins{1, 0}),
        el::Alignment::CenterLeft,
        el::cterm::BlockStyle::reset());
    if (!initial) {
        terminal()->moveUp(el::BlockCoordinate{1});
    }
    terminal()->write(buffer);
    terminal()->flush();
}

void ReadLineApp::finishReadLine(const el::cterm::ReadLineResult &result) {
    _updateTimer->stop();
    _readLine->stop();
    terminal()->setCursorVisible(true);
    terminal()->printLine("ReadLine status: "_el, statusText(result.status()));
    terminal()->printLine("Returned text: "_el, result.data());
    terminal()->flush();
    quit();
}

auto ReadLineApp::statusText(const el::cterm::ReadLineStatus &status) -> el::String {
    if (status.isCommitted()) {
        return "committed"_el;
    }
    if (status.isCancelled()) {
        return "cancelled"_el;
    }
    if (status.isTimeout()) {
        return "timeout"_el;
    }
    return "idle"_el;
}

}
