// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ConfigCheckApp.hpp"

#include "ConfigurationBatch.hpp"

#include <erbsland/ArgumentCount.hpp>
#include <erbsland/DiagnosticHelper.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/EventLoopErrorPolicy.hpp>
#include <erbsland/EventLoopOptions.hpp>
#include <erbsland/ExitCode.hpp>
#include <erbsland/ManagedEventThread.hpp>
#include <erbsland/OptionError.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionType.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/PlainTextRenderer.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringList.hpp>
#include <erbsland/TextDocument.hpp>

namespace demo {

using namespace el::text::literals;

void ConfigCheckApp::initialize() {
    info().setApplicationName("Configuration Check"_el);
}

void ConfigCheckApp::registerCommandLineOptions(const el::OptionsPtr &options) {
    options->setHelpDescription("Check ELCL files and report every failed file, or stop at an error limit."_el);
    options->addOption(cFilesOption)
        .setRequired()
        .setMaximum(el::ArgumentCount::infinite())
        .setValueName("file"_el)
        .setHelpTitle("Configuration files to check"_el)
        .setHelpDescription(
            "Files are parsed in argument order with Core's default include and access rules. "_el
            "This checks ELCL parsing, not an application-specific schema."_el);
    options->addOption({"--max-errors"_el, cMaximumErrorsOption})
        .setType(el::OptionType::Integer)
        .setDefaultValue(0)
        .setValueName("count"_el)
        .setHelpTitle("Stop after this many failed files"_el)
        .setHelpDescription(
            "Zero checks all files. A positive limit leaves remaining files unchecked. "_el
            "Each file contributes at most its first parser error."_el);
}

void ConfigCheckApp::handleCommandLineOptions(const el::OptionValuesPtr &values) {
    const auto maximumErrors = values->getInteger(cMaximumErrorsOption);
    if (maximumErrors < 0) {
        throw el::OptionError{"The error limit must be non-negative."_el, values->value(cMaximumErrorsOption)};
    }
    _maximumErrors = el::ItemCount::fromIntegerOrThrow(maximumErrors);
    for (const auto &text : values->getTextList(cFilesOption)) {
        _paths.append(el::Path::fromNativeOrThrow(text));
    }
}

auto ConfigCheckApp::main() -> el::ExitCode {
    // Declare the batch first so the worker and its callbacks are destroyed before their borrowed state.
    auto batch = ConfigurationBatch{_paths, _maximumErrors};
    const auto worker = createEventThread(el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::Store));
    batch.schedule(worker->workerEventLoop());
    worker->start();
    worker->join();

    // Joining makes worker results safe to read; it does not report the stored failures.
    const auto failures = reportFailures(worker->workerEventLoop());
    for (const auto &path : batch.passedPaths()) {
        el::io::printLine(el::StringFormat{"OK  {:text:esc=cpp}"_el}.build(path.toNativeString()));
    }
    const auto skipped = _paths.count() - batch.checkedCount();
    el::io::printLine(
        el::StringFormat{"Checked {} of {} files: {} passed, {} failed, {} skipped."_el}.build(
            batch.checkedCount(), _paths.count(), batch.passedPaths().count(), failures, skipped));
    if (!failures.isZero()) {
        return el::ExitCode::failure();
    }
    return el::ExitCode::success();
}

auto ConfigCheckApp::reportFailures(el::EventLoop &loop) -> el::ItemCount {
    auto failures = el::ItemCount{};
    while (const auto error = loop.takeError()) {
        const auto document = el::DiagnosticHelper::documentFromError(error);
        el::stream::stdErr()->writeLine(el::PlainTextRenderer{document}.build());
        ++failures;
    }
    el::stream::stdErr()->flush();
    return failures;
}

}
