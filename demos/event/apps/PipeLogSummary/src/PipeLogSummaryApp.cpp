// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PipeLogSummaryApp.hpp"

#include "LogCounts.hpp"
#include "PipeBackend.hpp"

#include <erbsland/EventLoop.hpp>
#include <erbsland/EventTimer.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/StandardStreams.hpp>

namespace demo {

using namespace el::text::literals;

void PipeLogSummaryApp::initialize() {
    info().setApplicationName("Pipe Log Summary"_el);
}

void PipeLogSummaryApp::registerCommandLineOptions(const el::OptionsPtr &options) {
    options->setHelpDescription("Count log lines by severity while a pipeline is running."_el);
    options->addOption(cFile)
        .setHelpTitle("Input file"_el)
        .setHelpDescription("Read this file to EOF. Omit it to read piped standard input."_el);
}

void PipeLogSummaryApp::handleCommandLineOptions(const el::OptionValuesPtr &values) {
    if (values->value(cFile)) {
        _file = el::Path::fromNativeOrThrow(values->getText(cFile));
    }
}

auto PipeLogSummaryApp::main() -> el::ExitCode {
    auto counts = LogCounts{};
    const auto driver = std::make_shared<PosixPipeDriver>(_file);
    const auto loop =
        el::EventLoop::create(driver, el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::Quit));
    const auto timer = loop->createTimer([&counts]() -> void { counts.report("Running"_el); });
    loop->registerBackend(std::make_unique<PipeBackend>(eventRegistry(), [&](const el::ByteBlock &bytes) -> void {
        counts.consume(bytes);
        if (bytes.isEmpty()) {
            timer->stop();
            counts.report("Final"_el);
            loop->quit();
        }
    }));
    timer->startFixedRate(el::Seconds{1});
    loop->run();
    if (const auto error = loop->takeError()) {
        std::rethrow_exception(error);
    }
    return el::ExitCode::success();
}

}
