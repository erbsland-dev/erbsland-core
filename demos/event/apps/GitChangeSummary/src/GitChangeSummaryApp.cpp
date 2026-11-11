// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "GitChangeSummaryApp.hpp"

#include <DemoTerminal.hpp>
#include <erbsland/ApplicationError.hpp>
#include <erbsland/EnvironmentVariables.hpp>
#include <erbsland/EventLoopErrorPolicy.hpp>
#include <erbsland/EventLoopOptions.hpp>
#include <erbsland/EventTarget.hpp>
#include <erbsland/OptionError.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/PathResolveMode.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringList.hpp>
#include <erbsland/SubprocessEventEditor.hpp>
#include <erbsland/SubprocessOptions.hpp>
#include <erbsland/SubprocessOutputMode.hpp>

namespace demo {

using namespace el::text::literals;

void GitChangeSummaryApp::initialize() {
    DemoTerminal::enable(*this);
    info().setApplicationName("Git Change Summary"_el);
}

void GitChangeSummaryApp::registerCommandLineOptions(const el::OptionsPtr &options) {
    options->addOption(cDirectoryOption)
        .setDefaultValue("."_el)
        .setValueName("directory"_el)
        .setHelpTitle("Summarize repository changes by directory."_el)
        .setHelpDescription("Defaults to the current directory. Git must be available on PATH."_el);
}

void GitChangeSummaryApp::handleCommandLineOptions(const el::OptionValuesPtr &values) {
    _directory =
        el::Path::fromNativeOrThrow(values->getText(cDirectoryOption)).resolveOrThrow(el::PathResolveMode::Physical);
    if (!_directory.info().isDirectory()) {
        throw el::OptionError{"Expected a repository directory."_el};
    }
}

auto GitChangeSummaryApp::main() -> el::ExitCode {
    auto processOptions = el::SubprocessOptions{};
    processOptions.setInheritStandardInput(false)
        .setStandardOutputMode(el::SubprocessOutputMode::Capture)
        .setStandardErrorMode(el::SubprocessOutputMode::Capture)
        .setRemoveOutputBom(false);
    const auto arguments = el::StringList{
        "--no-optional-locks"_el,
        "-C"_el,
        _directory.toNativeString(),
        "status"_el,
        "--porcelain=v1"_el,
        "-z"_el,
        "--untracked-files=all"_el,
    };
    // Preparation needs no event loop. The source has no owner yet.
    _process = el::Subprocess::create(findGit(), arguments, processOptions);
    auto loopOptions = el::EventLoopOptions{};
    loopOptions.setErrorPolicy(el::EventLoopErrorPolicy::FailApplication);
    _worker = createEventThread(loopOptions);
    _worker->start();
    _worker->loop()->invoke([this]() -> void { onStart(); });
    _worker->join();
    // Joining completes the handover of worker-owned results to procedural main.
    if (_exitCode.isSuccess()) {
        printReport();
    }
    return _exitCode;
}

void GitChangeSummaryApp::onStart() {
    // The first events() call binds the source permanently to this worker loop.
    _process->events()
        .onStandardOutput([this](const el::String &chunk) -> void { onStandardOutput(chunk); })
        .onExit([this](const el::ExitCode code) -> void { onExit(code); })
        .onError([this](const el::PlatformErrorContext &error) -> void { onError(error); })
        .onOutputTruncated([this]() -> void { onOutputTruncated(); })
        .onFinal([this]() -> void { onFinal(); });
    _process->start();
}

void GitChangeSummaryApp::onStandardOutput(const el::String &chunk) {
    _report.append(chunk);
}

void GitChangeSummaryApp::onExit(const el::ExitCode code) {
    if (code.isFailure()) {
        throw el::ApplicationError{"Git status failed."_el, _process->standardError()};
    }
    if (_process->wasStandardOutputTruncated() || _process->wasStandardErrorTruncated()) {
        onOutputTruncated();
    }
    _report.finish();
    _exitCode = el::ExitCode::success();
}

void GitChangeSummaryApp::onError(const el::PlatformErrorContext &error) {
    throw el::ApplicationError{"Git subprocess failed."_el, error.toString()};
}

void GitChangeSummaryApp::onOutputTruncated() {
    throw el::ApplicationError{"Git output was truncated; no complete report is available."_el};
}

void GitChangeSummaryApp::onFinal() {
    _worker->quit();
}

void GitChangeSummaryApp::cleanup() noexcept {
    // Application failure handling has joined managed workers before cleanup.
    _process.reset();
    _worker.reset();
}

auto GitChangeSummaryApp::findGit() -> el::Path {
    for (const auto &candidate : el::EnvironmentVariables{}.executableCandidatesOrThrow("git"_el)) {
        const auto path = candidate.resolve();
        if (!path.isEmpty() && path.info().isRegularFile() && path.info().isExecutable()) {
            return path;
        }
    }
    throw el::ApplicationError{"Git is unavailable."_el, "Install Git and make it available on PATH."_el};
}

void GitChangeSummaryApp::printReport() const {
    if (_report.directories().count().isZero()) {
        el::io::printLine("Working tree is clean."_el);
        return;
    }
    el::io::printLine("Staged  Unstaged  Untracked  Conflicted  Directory"_el);
    const auto row = el::StringFormat{"{:>6}  {:>8}  {:>9}  {:>10}  {:text:esc=cpp}"_el};
    for (const auto &[directory, counts] : _report.directories()) {
        el::io::printLine(row.build(counts.staged, counts.unstaged, counts.untracked, counts.conflicted, directory));
    }
}

}
